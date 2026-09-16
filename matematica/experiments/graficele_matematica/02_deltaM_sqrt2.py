import math
import numpy as np
import matplotlib.pyplot as plt

def delta_M(R, M):
    """
    Calculează δ_M(R) = min |p - Rq|
    pentru perechi întregi nenule fezabile în [-M,M]^3.
    Pentru R > 0, candidații relevanți pot fi construiți din
    q și cel mai apropiat p de Rq.
    """
    best = float("inf")

    # |q| <= 2M
    for q in range(-2*M, 2*M + 1):
        if q == 0:
            continue

        rq = R * q
        for p in {math.floor(rq), math.ceil(rq)}:
            if p == 0 and q == 0:
                continue

            width = max(0, p, q) - min(0, p, q)
            if width <= 2*M:
                defect = abs(p - R*q)
                if defect < best:
                    best = defect

    return best

R = math.sqrt(2)
M_values = np.arange(1, 501)
deltas = np.array([delta_M(R, int(M)) for M in M_values])

# liminf M*δ_M(√2) = 1/4.
scaled = M_values * deltas

plt.figure(figsize=(9, 5.5))
plt.plot(M_values, scaled, linewidth=1.4,
         label="M · δ_M(√2)")
plt.axhline(1/4, linestyle="--", linewidth=1.5,
            label="constanta teoretică 1/4")

plt.xlabel("M")
plt.ylabel("M · δ_M(√2)")
plt.title("Rigiditatea de ordinul întâi pentru R = √2")
plt.legend()
plt.grid(alpha=0.25)
plt.tight_layout()
plt.show()
