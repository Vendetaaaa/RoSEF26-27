import numpy as np
import matplotlib.pyplot as plt

# Exp din studiu:
# κ₁ = √2, κ₂ = 1, κ₃ = 0
# R = √2
# q = 41, p = 58
# m₃ = 0, m₂ = 41, m₁ = 58.
#
# d2 ∩ d3 este (0, -41).
# d1 ∩ d2 V d3 = ∅ ∵ 58 - 41√2 ≈ 0.017244.
#
# Avem near-concurență, nu totală

kappa = [np.sqrt(2), 1, 0]
m = [58, 41, 0]

y = np.linspace(-44, -38, 600)
x = [k * y + mi for k, mi in zip(kappa, m)]

fig, ax = plt.subplots(figsize=(9, 6))

for i in range(3):
    ax.plot(x[i], y, linewidth=2,
            label=f"Dreapta {i+1}")

# d2 ∩ d3
x0, y0 = 0, -41
ax.scatter([x0], [y0], s=70, zorder=5)
ax.annotate("(0, −41): intersecția D₂ și D₃",
            (x0, y0), xytext=(0.5, -40.2),
            arrowprops=dict(arrowstyle="->"))

# d1 ∩ y{-41}
x1 = np.sqrt(2) * (-41) + 58
defect = 58 - 41 * np.sqrt(2)

ax.scatter([x1], [y0], s=55, zorder=5)
ax.annotate(
    f"abatere orizontală ≈ {abs(defect):.6f}",
    (x1, y0), xytext=(1.2, -42.2),
    arrowprops=dict(arrowstyle="->")
)

ax.axhline(0, linewidth=0.8)
ax.axvline(0, linewidth=0.8)
ax.set_xlabel("x")
ax.set_ylabel("y")
ax.set_title("Near-concurență pentru R = √2")
ax.grid(True, alpha=0.25)
ax.legend()
fig.tight_layout()

fig.savefig("02_near_concurenta_sqrt2.png", dpi=200, bbox_inches="tight")
plt.show()
