import numpy as np
import matplotlib.pyplot as plt
from matplotlib.widgets import Slider
from scipy import signal


# ============================================================
# Basic settings
# ============================================================

fs = 48000.0
nyq = fs * 0.5

N = 6
rp = 0.1      # passband ripple, dB
rs = 60.0     # stopband attenuation, dB

f_min = 20.0
f_max = nyq

cutoff_target_init = 5000.0
cutoff_base_init = 10000.0

# Frequency grid for plotting
freqs = np.geomspace(20.0, nyq * 0.999999, 4096)


# ============================================================
# Helpers
# ============================================================

def clamp_cutoff(fc):
    """
    scipy.signal.ellip requires 0 < Wn < fs/2.
    The UI allows 24000 Hz, but internally we clamp slightly below Nyquist.
    """
    return float(np.clip(fc, 20.0, nyq * 0.999999))


def hz_to_log_slider(f):
    return np.log10(f)


def log_slider_to_hz(v):
    return 10.0 ** v


def db(h):
    return 20.0 * np.log10(np.maximum(np.abs(h), 1e-12))


def design_direct_ellip(cutoff):
    """
    Direct digital elliptic lowpass design.
    Returns z, p, k in scipy zpk format.
    """
    cutoff = clamp_cutoff(cutoff)
    return signal.ellip(
        N,
        rp,
        rs,
        cutoff,
        btype="lowpass",
        fs=fs,
        output="zpk",
    )


def apf_k(cutoff_target, cutoff_base):
    """
    Lowpass-to-lowpass first-order allpass transform parameter.

    This follows your implementation:

        k = sin(pi * (target - base) / fs)
          / sin(pi * (target + base) / fs)

    For the substitution:

        z' = (k + z) / (1 + k z)
    """
    cutoff_target = clamp_cutoff(cutoff_target)
    cutoff_base = clamp_cutoff(cutoff_base)

    num = np.sin(np.pi * (cutoff_target - cutoff_base) / fs)
    den = np.sin(np.pi * (cutoff_target + cutoff_base) / fs)

    # Avoid division by zero in pathological slider positions.
    if abs(den) < 1e-15:
        den = np.copysign(1e-15, den)

    return num / den


def apf_transform_zpk(z_base, p_base, g_base, k):
    """
    Apply:

        z' = (k + z) / (1 + k z)

    to a zpk filter H_base(z').

    If r is a zero or pole in the z' plane, solve:

        r = (k + z) / (1 + k z)

    Then:

        z = (r - k) / (1 - r k)

    So every zero and pole is mapped by:

        r -> (r - k) / (1 - r k)

    Gain is then normalized so that H(DC) is preserved.
    Since z = 1 maps to z' = 1, this is a natural normalization
    for lowpass filters.
    """

    z_base = np.asarray(z_base, dtype=np.complex128)
    p_base = np.asarray(p_base, dtype=np.complex128)

    z_new = (z_base - k) / (1.0 - z_base * k)
    p_new = (p_base - k) / (1.0 - p_base * k)

    # Preserve DC gain.
    # scipy zpk convention:
    # H(z) = g * prod(z - z_i) / prod(z - p_i)
    base_dc = g_base * np.prod(1.0 - z_base) / np.prod(1.0 - p_base)
    raw_dc = np.prod(1.0 - z_new) / np.prod(1.0 - p_new)

    g_new = base_dc / raw_dc
    g_new = np.real_if_close(g_new)

    return z_new, p_new, g_new


def design_apf_warped_ellip(cutoff_target, cutoff_base):
    """
    Design base elliptic filter at cutoff_base, then move its cutoff
    to cutoff_target by first-order allpass substitution.
    """
    z_base, p_base, g_base = design_direct_ellip(cutoff_base)
    k = apf_k(cutoff_target, cutoff_base)
    return apf_transform_zpk(z_base, p_base, g_base, k)


def responses(cutoff_target, cutoff_base):
    """
    Compute direct-design response and APF-warped response.
    """
    cutoff_target = clamp_cutoff(cutoff_target)
    cutoff_base = clamp_cutoff(cutoff_base)

    z_d, p_d, g_d = design_direct_ellip(cutoff_target)
    _, h_direct = signal.freqz_zpk(z_d, p_d, g_d, worN=freqs, fs=fs)

    z_w, p_w, g_w = design_apf_warped_ellip(cutoff_target, cutoff_base)
    _, h_warped = signal.freqz_zpk(z_w, p_w, g_w, worN=freqs, fs=fs)

    return db(h_direct), db(h_warped)


# ============================================================
# Initial plot
# ============================================================

mag_direct, mag_warped = responses(cutoff_target_init, cutoff_base_init)

fig, ax = plt.subplots(figsize=(10, 6))
plt.subplots_adjust(bottom=0.25)

line_direct, = ax.plot(
    freqs,
    mag_direct,
    color="red",
    linewidth=1.5,
    label="Direct digital elliptic design",
)

line_warped, = ax.plot(
    freqs,
    mag_warped,
    color="blue",
    linewidth=1.5,
    linestyle="--",
    label="APF-warped from base cutoff",
)

ax.set_xscale("log")
ax.set_xlim(20.0, 24000.0)
ax.set_ylim(-80.0, 20.0)

ax.set_xlabel("Frequency [Hz]")
ax.set_ylabel("Magnitude [dB]")
ax.set_title("6th-order digital elliptic LPF: direct design vs APF cutoff transform")
ax.grid(True, which="both", alpha=0.3)
ax.legend(loc="best")


# ============================================================
# Sliders with logarithmic feel
# ============================================================

slider_min = hz_to_log_slider(20.0)
slider_max = hz_to_log_slider(24000.0)

ax_target = plt.axes([0.15, 0.12, 0.75, 0.03])
ax_base = plt.axes([0.15, 0.07, 0.75, 0.03])

slider_target = Slider(
    ax=ax_target,
    label="cutoff_target",
    valmin=slider_min,
    valmax=slider_max,
    valinit=hz_to_log_slider(cutoff_target_init),
)

slider_base = Slider(
    ax=ax_base,
    label="cutoff_base",
    valmin=slider_min,
    valmax=slider_max,
    valinit=hz_to_log_slider(cutoff_base_init),
)

# Make slider text show Hz instead of log10(Hz)
slider_target.valtext.set_text(f"{cutoff_target_init:.1f} Hz")
slider_base.valtext.set_text(f"{cutoff_base_init:.1f} Hz")


def update(_):
    cutoff_target = log_slider_to_hz(slider_target.val)
    cutoff_base = log_slider_to_hz(slider_base.val)

    cutoff_target = clamp_cutoff(cutoff_target)
    cutoff_base = clamp_cutoff(cutoff_base)

    mag_direct, mag_warped = responses(cutoff_target, cutoff_base)

    line_direct.set_ydata(mag_direct)
    line_warped.set_ydata(mag_warped)

    slider_target.valtext.set_text(f"{cutoff_target:.1f} Hz")
    slider_base.valtext.set_text(f"{cutoff_base:.1f} Hz")

    k = apf_k(cutoff_target, cutoff_base)
    ax.set_title(
        "6th-order digital elliptic LPF: direct design vs APF cutoff transform\n"
        f"target = {cutoff_target:.1f} Hz, base = {cutoff_base:.1f} Hz, k = {k:.8f}"
    )

    fig.canvas.draw_idle()


slider_target.on_changed(update)
slider_base.on_changed(update)

plt.show()