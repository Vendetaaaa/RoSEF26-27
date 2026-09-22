import numpy as np
import matplotlib.pyplot as plt

# κ = (2, 1, 0), deci R = (2-0)/(1-0) = 2
# 1 = (1,1,1), κ = (2,1,0)
# Vκ = span_R{1, κ}
#
# Pentru R = 2:
# (m1-m3, m2-m3) = t(2,1)

from mpl_toolkits.mplot3d import Axes3D  # noqa: F401

kappa = np.array([2.0, 1.0, 0.0])
one = np.array([1.0, 1.0, 1.0])

fig = plt.figure(figsize=(9, 7))
ax = fig.add_subplot(111, projection="3d")

# m = a*1 + t*kappa
vals = range(-4, 5)
points = []
for a in vals:
    for t in vals:
        m = a * one + t * kappa
        points.append(m)

points = np.array(points)
ax.scatter(points[:, 0], points[:, 1], points[:, 2],
           s=20, alpha=0.75, label="puncte întregi concurente")

# Planul Vκ.
a_grid = np.linspace(-4.5, 4.5, 30)
t_grid = np.linspace(-4.5, 4.5, 30)
A, T = np.meshgrid(a_grid, t_grid)
X = A + 2*T
Y = A + T
Z = A

ax.plot_surface(X, Y, Z, alpha=0.18)

# Dir de rezonanță (2,1,0)
u = np.linspace(-5, 5, 100)
ax.plot(2*u, u, np.zeros_like(u), linewidth=2,
        label="direcția (2,1,0)")

ax.set_xlabel("m₁")
ax.set_ylabel("m₂")
ax.set_zlabel("m₃")
ax.set_title("Planul Vκ = spanℝ{1, κ} și rețeaua de concurență")
ax.legend()
plt.tight_layout()
plt.show()
