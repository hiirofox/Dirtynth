import math
import numpy as np
import torch
import matplotlib.pyplot as plt

# =========================
# Config
# =========================

FS = 48000.0

N_APF = 4             # 每条链的一阶 APF 数量
STEPS = 1000          # Adam 迭代次数
LR = 0.05

F_MIN = 30.0          # 优化频段下限，不要用 0 Hz
F_MAX = 20000.0       # 优化频段上限，不要碰 Nyquist
N_FREQ = 2048

TARGET_SIGN = -1      # -1 => im - re = -90 deg; +1 => im - re = +90 deg
K_LIMIT = 0.999995       # k = K_LIMIT * tanh(u)，避免 k 太接近 +/-1

SEED = 1
PLOT_EVERY = 50

DTYPE = torch.float64
DEVICE = "cpu"        # 可以改成 "cuda"


# =========================
# Helpers
# =========================

def wrap_pi(x):
    """Wrap radians to [-pi, pi]."""
    return torch.atan2(torch.sin(x), torch.cos(x))


def allpass_response(k_vec, z1):
    """
    一阶 APF:
        y[n] = k * (x[n] - k*z[n-1]) + z[n-1]
        z[n] = x[n] - k*z[n-1]

    传函:
        H(z) = (k + z^-1) / (1 + k z^-1)
    """
    H = torch.ones_like(z1)
    for k in k_vec:
        kc = k.to(z1.dtype)
        H = H * (kc + z1) / (1.0 + kc * z1)
    return H


def phase_error_and_diff(k_re, k_im, z1, target):
    """
    re branch:
        APF chain

    im branch:
        z^-1 -> APF chain

    diff:
        phase(im / re)
    """
    H_re = allpass_response(k_re, z1*z1)
    H_im = z1*allpass_response(k_im, z1*z1) 

    ratio = H_im / H_re
    diff = torch.atan2(ratio.imag, ratio.real)

    err = wrap_pi(diff - target)
    return err, diff


# =========================
# Frequency grid
# =========================

torch.manual_seed(SEED)
np.random.seed(SEED)

f_max = min(F_MAX, FS * 0.49)
freq_np = np.geomspace(F_MIN, f_max, N_FREQ)

w = torch.tensor(2.0 * np.pi * freq_np / FS, dtype=DTYPE, device=DEVICE)

# z^-1 = exp(-jw)
z1 = torch.polar(torch.ones_like(w), -w)

# 对数频率网格下等权。也可以自己改成 perceptual/linear 权重。
weights = torch.ones_like(w)
weights = weights / weights.mean()

target = TARGET_SIGN * math.pi / 2.0


# =========================
# Parameters
# =========================

# 不直接优化 k，而是优化 u，再 tanh 到稳定区间。
u_re = torch.nn.Parameter(torch.randn(N_APF, dtype=DTYPE, device=DEVICE) * 0.25)
u_im = torch.nn.Parameter(torch.randn(N_APF, dtype=DTYPE, device=DEVICE) * 0.25)

optimizer = torch.optim.Adam([u_re, u_im], lr=LR)


# =========================
# Live plot
# =========================

plt.ion()
fig, ax = plt.subplots(figsize=(9, 5))

line_phase, = ax.semilogx(freq_np, np.zeros_like(freq_np), label="phase(im) - phase(re)")
ax.axhline(TARGET_SIGN * 90.0, linestyle="--", linewidth=1.0, label="target")

ax.set_xlabel("Frequency [Hz]")
ax.set_ylabel("Phase difference [deg]")
ax.set_title("IIR Hilbert APF-chain optimization")
ax.grid(True, which="both", alpha=0.3)
ax.legend()
ax.set_ylim(TARGET_SIGN * 90.0 - 80.0, TARGET_SIGN * 90.0 + 80.0)


# =========================
# Optimize
# =========================
# =========================
# Optimize: Adam then LBFGS
# =========================

ADAM_STEPS = STEPS//4
LBFGS_STEPS = STEPS

best_loss = float("inf")
best_re = None
best_im = None


def current_k():
    k_re = K_LIMIT * torch.tanh(u_re)
    k_im = K_LIMIT * torch.tanh(u_im)
    return k_re, k_im


def compute_loss():
    k_re, k_im = current_k()
    err, diff = phase_error_and_diff(k_re, k_im, z1, target)
    loss = torch.mean(weights * err * err)
    return loss, err, diff


def update_best(loss_value):
    global best_loss, best_re, best_im

    if loss_value < best_loss:
        with torch.no_grad():
            k_re, k_im = current_k()
            best_loss = loss_value
            best_re = k_re.detach().cpu().numpy().copy()
            best_im = k_im.detach().cpu().numpy().copy()


def update_plot(stage_name, step, total_steps, loss_value):
    with torch.no_grad():
        loss_now, err_now, diff_now = compute_loss()

        centered_diff = wrap_pi(diff_now - target) + target
        phase_deg = centered_diff.detach().cpu().numpy() * 180.0 / math.pi

        err_deg = err_now.detach().cpu().numpy() * 180.0 / math.pi
        rms_deg = float(np.sqrt(np.mean(err_deg * err_deg)))
        max_deg = float(np.max(np.abs(err_deg)))

        line_phase.set_ydata(phase_deg)
        ax.set_title(
            f"{stage_name} {step}/{total_steps} | "
            f"loss={loss_value:.6g} | "
            f"rms={rms_deg:.3f} deg | "
            f"max={max_deg:.3f} deg"
        )

        fig.canvas.draw()
        fig.canvas.flush_events()
        plt.pause(0.001)


# =========================
# Stage 1: Adam
# =========================

adam = torch.optim.Adam([u_re, u_im], lr=LR)

for step in range(ADAM_STEPS + 1):
    adam.zero_grad()

    loss, err, diff = compute_loss()
    loss.backward()
    adam.step()

    loss_value = float(loss.detach().cpu())
    update_best(loss_value)

    if step % PLOT_EVERY == 0 or step == ADAM_STEPS:
        update_plot("Adam", step, ADAM_STEPS, loss_value)


# =========================
# Stage 2: LBFGS
# =========================

lbfgs = torch.optim.LBFGS(
    [u_re, u_im],
    lr=0.8,
    max_iter=1,             # 每次 step 只做一轮，外层循环控制轮数
    max_eval=8,
    history_size=64,
    line_search_fn="strong_wolfe",
    tolerance_grad=0.0,
    tolerance_change=0.0,
)

for step in range(1, LBFGS_STEPS + 1):

    def closure():
        lbfgs.zero_grad()
        loss, err, diff = compute_loss()
        loss.backward()
        return loss

    loss = lbfgs.step(closure)

    # 注意：LBFGS 返回的 loss 可能是 line search 前的 loss，
    # 所以这里重新计算一次当前参数下的真实 loss。
    with torch.no_grad():
        loss_now, err_now, diff_now = compute_loss()
        loss_value = float(loss_now.detach().cpu())

    update_best(loss_value)

    if step % PLOT_EVERY == 0 or step == LBFGS_STEPS:
        update_plot("LBFGS", step, LBFGS_STEPS, loss_value)

# =========================
# Print result
# =========================

print()
print("Best loss:", best_loss)

# 用 best coeffs 再算一次最终误差
with torch.no_grad():
    k_re_t = torch.tensor(best_re, dtype=DTYPE, device=DEVICE)
    k_im_t = torch.tensor(best_im, dtype=DTYPE, device=DEVICE)
    err_final, diff_final = phase_error_and_diff(k_re_t, k_im_t, z1, target)

    err_deg = err_final.detach().cpu().numpy() * 180.0 / math.pi
    rms_deg = float(np.sqrt(np.mean(err_deg * err_deg)))
    max_deg = float(np.max(np.abs(err_deg)))

print(f"RMS phase error: {rms_deg:.6f} deg")
print(f"Max phase error: {max_deg:.6f} deg")
print()

print("re_k = {")
for v in best_re:
    print(f"    {v:.9f}f,")
print("};")

print()

print("im_k = {")
for v in best_im:
    print(f"    {v:.9f}f,")
print("};")

plt.ioff()
plt.show()