import math
import numpy as np
import matplotlib.pyplot as plt

def delta_M(R, M):
    best = float("inf")

    for q in range(-2*M, 2*M + 1):
        if q == 0:
            continue

        rq = R * q
        for p in {math.floor(rq), math.ceil(rq)}:
            width = max(0, p, q) - min(0, p, q)
            if width <= 2*M:
                best = min(best, abs(p - R*q))

    return best

phi = (1 + math.sqrt(5)) / 2
M_values = np.arange(1, 501)
deltas = np.array([delta_M(phi, int(M)) for M in M_values])
scaled = M_values * deltas

# liminf M*δ_M(φ) = φ/(2√5).
theoretical_constant = phi / (2 * math.sqrt(5))

plt.figure(figsize=(9, 5.5))
plt.plot(M_values, scaled, linewidth=1.4,
         label="M · δ_M(φ)")
plt.axhline(theoretical_constant, linestyle="--", linewidth=1.5,
            label="φ/(2√5)")

plt.xlabel("M")
plt.ylabel("M · δ_M(φ)")
plt.title("Apropierea de concurență pentru raportul de aur φ")
plt.legend()
plt.grid(alpha=0.25)
plt.tight_layout()
plt.show()
