import math
import numpy as np
import matplotlib.pyplot as plt

def delta_M_rational(R, M):
    best = float("inf")

    for q in range(-2*M, 2*M + 1):
        if q == 0:
            continue
        p_exact = R * q
        candidates = {math.floor(p_exact), math.ceil(p_exact)}

        for p in candidates:
            width = max(0, p, q) - min(0, p, q)
            if width <= 2*M:
                best = min(best, abs(p - R*q))

    return best

R = 3/2
M_values = np.arange(1, 12)
deltas = np.array([delta_M_rational(R, int(M)) for M in M_values])

plt.figure(figsize=(9, 5.5))
plt.step(M_values, deltas, where="mid", linewidth=2,
          label="δ_M(3/2)")
plt.axvline(2, linestyle="--", linewidth=1.4,
            label="M = 2: apare (p,q) = (3,2)")

plt.xlabel("M")
plt.ylabel("δ_M(3/2)")
plt.title("Rezonanță rațională: apariția defectului zero")
plt.xticks(M_values)
plt.legend()
plt.grid(alpha=0.25)
plt.tight_layout()
plt.show()
