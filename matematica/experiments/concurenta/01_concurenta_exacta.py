import numpy as np
import matplotlib.pyplot as plt

# Luăm un exp R rațional:
#
# Alegem κ₁ = 2, κ₂ = 1, κ₃ = 0, deci R = 2; m₃ = 0, q = 1 și p = 2
# m₁ = 2, m₂ = 1, m₃ = 0.
#
# Cele trei drepte sunt:
# x - 2y = 2
# x - y  = 1
# x       = 0
#
# Toate trec prin punctul P(0, -1).

kappa = [2, 1, 0]
m = [2, 1, 0]

y = np.linspace(-3, 2, 500)

# x - ky = m <=> x = ky + m
x = [k * y + mi for k, mi in zip(kappa, m)]

fig, ax = plt.subplots(figsize=(8, 6))

for i in range(3):
    ax.plot(x[i], y, linewidth=2, label=f"Dreapta {i+1}: κ{i+1}={kappa[i]}, m{i+1}={m[i]}")

# P(0, -1)
ax.scatter([0], [-1], s=70, zorder=5)
ax.annotate("P = (0, −1)", (0, -1), xytext=(0.35, -0.65),
            arrowprops=dict(arrowstyle="->"))

ax.axhline(0, linewidth=0.8)
ax.axvline(0, linewidth=0.8)
ax.set_xlabel("x")
ax.set_ylabel("y")
ax.set_title("Concurență exactă: rezonanță aritmetică rațională")
ax.grid(True, alpha=0.25)
ax.legend()
fig.tight_layout()

fig.savefig("01_concurenta_exacta.png", dpi=200, bbox_inches="tight")
plt.show()
