import math
import numpy as np
import matplotlib.pyplot as plt

R1 = math.sqrt(2)
R3 = math.sqrt(3)

q_values = np.arange(1, 301)

# max(|p1 - R1 q|, |p3 - R3 q|).
simultaneous_defect = []

for q in q_values:
    p1 = round(R1 * q)
    p3 = round(R3 * q)

    d1 = abs(p1 - R1*q)
    d3 = abs(p3 - R3*q)
    simultaneous_defect.append(max(d1, d3))

simultaneous_defect = np.array(simultaneous_defect)

# q=41, p1=58, p3=71.
q0 = 41
p1_0 = 58
p3_0 = 71
d1_0 = abs(p1_0 - R1*q0)
d3_0 = abs(p3_0 - R3*q0)
d0 = max(d1_0, d3_0)

plt.figure(figsize=(9, 5.5))
plt.plot(q_values, simultaneous_defect, linewidth=1.2,
         label="max(|p₁−√2q|, |p₃−√3q|)")
plt.scatter([q0], [d0], s=55,
            label=f"q=41, defect≈{d0:.4f}")

plt.xlabel("q")
plt.ylabel("defect simultan")
plt.title("Aproximare simultană pentru k = 4")
plt.legend()
plt.grid(alpha=0.25)
plt.tight_layout()
plt.show()
