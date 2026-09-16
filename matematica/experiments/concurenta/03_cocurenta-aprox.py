import numpy as np
import matplotlib.pyplot as plt

# Avem 4 linii:
# κ₁ = √2, κ₂ = 1, κ₃ = √3, κ₄ = 0
# q = 41
# m₄ = 0, m₂ = 41, m₁ = 58, m₃ = 71
#
# d2 ∩ d4 este P(0, -41)
# d1, d3 sunt aprox concurente
#
# Defecte:
# 58 - 41√2 ≈  0.017244
# 71 - 41√3 ≈ -0.014083

kappa = [np.sqrt(2), 1, np.sqrt(3), 0]
m = [58, 41, 71, 0]

y = np.linspace(-44, -38, 700)
x = [k * y + mi for k, mi in zip(kappa, m)]

fig, ax = plt.subplots(figsize=(10, 6.5))

for i in range(4):
    ax.plot(x[i], y, linewidth=2, label=f"D{i+1}")

# d2 ∩ d4
x0, y0 = 0, -41
ax.scatter([x0], [y0], s=75, zorder=6)
ax.annotate(
    "P = (0, −41)\nD₂ ∩ D₄",
    (x0, y0),
    xytext=(1.0, -40.1),
    arrowprops=dict(arrowstyle="->")
)

# d1 ∩ d3 ∩ y{-41}
x1 = np.sqrt(2) * (-41) + 58
x3 = np.sqrt(3) * (-41) + 71

ax.scatter([x1, x3], [y0, y0], s=55, zorder=6)

ax.annotate(
    f"D₁: |defect| ≈ {abs(58 - 41*np.sqrt(2)):.6f}",
    (x1, y0),
    xytext=(-5.2, -42.4),
    arrowprops=dict(arrowstyle="->")
)

ax.annotate(
    f"D₃: |defect| ≈ {abs(71 - 41*np.sqrt(3)):.6f}",
    (x3, y0),
    xytext=(1.0, -43.0),
    arrowprops=dict(arrowstyle="->")
)

ax.axhline(0, linewidth=0.8)
ax.axvline(0, linewidth=0.8)
ax.set_xlabel("x")
ax.set_ylabel("y")
ax.set_title("Near-concurență simultană pentru patru familii")
ax.grid(True, alpha=0.25)
ax.legend()
fig.tight_layout()

fig.savefig("03_concurenta_aproximativa_k4.png", dpi=200, bbox_inches="tight")
plt.show()
