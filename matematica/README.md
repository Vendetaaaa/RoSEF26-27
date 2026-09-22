# Documentația matematică

Fișierul curent va dezvolta abstractul parții matematice și calculele computațională a proiectului.

# Structura /matematica

```text
matematica/
  experiments/
  README.md
```

# Abstract

Considerăm familii de drepte de forma:

x − κᵢy = mᵢ, unde i ∈ {1, 2, 3}, κᵢ sunt numere reale, iar mᵢ ∈ ℤ.

După eliminarea lui x, vom obține relația:

(m₁ − m₃)(κ₂ − κ₃) = (m₂ − m₃)(κ₁ − κ₃), unde notăm: 

</br>

R = (κ₁ − κ₃) / (κ₂ − κ₃) cu κ₂ ≠ κ₃,

p = m₁ − m₃,

q = m₂ − m₃.

</br>

Ecuația ajunge:
p = Rq (1)

Iar pentru (1), definim defectul perecheii de numere întregi (p, q) ca:
def_R(p, q) = p - Rq unde există o concureță dacă: def_R(p, q) = 0

> Așadar, ipoteza geometrică este transformată într-o problemă aritmetică de a analiza soluțiile întregi pentru relația (1).


---

</br>

</br>

**Dacă R este rațional** putem nota:

R = a / b; cu a, b ∈ ℤ și gcd(a, b) = 1.

p / q = a / b <=> bp = aq ∴ (∃)S = {(p, q) = t(a, b) / t ∈ ℤ }

Prin urmare, raționalizarea lui R, duce la existența unei familii infinite de soluții întregi.

**Dacă R este irațional** ecuația (1) nu poate fi satisfăcută pentru o pereche de întregi nenulă (p, q). Putem totuși căuta o aproximare cu:

|p − Rq| ≈ 0 unde vom avea nevoie de o concurență aproximativă pentru un domeniu bine definit, așadar vom căuta cât de aproape de zero poate atinge defectul?

---

</br>

</br>

Considerăm cutia: B_M = [−M, M]³ ∩ ℤ³.

De asemenea vom nota: w(p, q) = max{0, p, q} - min{0, p, q}.

Atunci (∃)w ∈ B_M dacă w(p, q) ≤ 2M, iar cu R irațional: δ_M(R) = min |p − Rq| unde δ_M(R) reprezintă cel mai mic defect de concurență obținut în domeniul finit considerat.

---

</br>

</br>

Pentru a crește rigiditatea, studiem viteza cu care δ_M(R) scade când M crește.

Așadar, introducem: 

D(R) = max{0, 1, R} − min{0, 1, R}.

Și definim:

L(R) = liminf q · ||qR||, a.î. ||qR|| reprezintă distanța de la qR la cel mai apropiat
număr întreg.

</br>

Rezultatul asimptotic principal studiat va fi:

liminf M · δ_M(R) = D(R) · L(R) / 2, relație ce conectează geometria near-concurenței cu proprietățile diofantice ale numărului R.

---

</br>

</br>

Rezultatul final va fi descris cât de bine poate fi aproximat un număr real prin numere raționale, va fi introdus μ(R), unde:

μ_geo(R) = 1 + limsup
log(1 / δ_M(R)) / log M.

ce rezultă în final:

μ_geo(R) = μ(R).

---

</br>

</br>

Pentru o familie cu k > 3 drepte, va fi necesar alegerea unei drepte de referință și introducerea raportului:

Rᵢ = (κᵢ − κ_k) / (κ₂ − κ_k). cu: q = m₂ − m_k, pᵢ = mᵢ − m_k,

iar condiția de concurență va deveni:

pᵢ = Rᵢq. => p − qR.

# Referințe

[1] J. W. S. Cassels, An Introduction to Diophantine Approximation, Cambridge University
Press, 1957.

[2] J. W. S. Cassels, An Introduction to the Geometry of Numbers, Springer, 1959.

[3] G. H. Hardy and E. M. Wright, An Introduction to the Theory of Numbers, 6th ed., Oxford
University Press, 2008.

[4] K. Ireland and M. Rosen, A Classical Introduction to Modern Number Theory, 2nd ed.,
Springer, 1990.

[5] A. Khintchine, “Einige Sätze über Kettenbrüche, mit Anwendungen auf die Theorie der
Diophantischen Approximationen,” Mathematische Annalen 92 (1924), 115–125.

[6] W. M. Schmidt, Diophantine Approximation, Lecture Notes in Mathematics, vol. 785,
Springer, 1980.

[7] G. Harman, Metric Number Theory, London Mathematical Society Monographs, New Series
18, Clarendon Press, 1998.

[8] V. G. Sprindzhuk, Metric Theory of Diophantine Approximations, V. H. Winston, 1979.

[9] Y. Bugeaud, Approximation by Algebraic Numbers, Cambridge University Press, 2004.

[10] K. F. Roth, “Rational approximations to algebraic numbers,” Mathematika 2 (1955), 1–20.

[11] H. Davenport and K. F. Roth, “Rational approximations to algebraic numbers,” Mathematika
2 (1955), 160–167.

[12] M. Waldschmidt, “Report on some recent advances in Diophantine approximation,”
arXiv:0908.3973, 2009.

[13] D. Badziahin, A. Pollington, and S. Velani, “On a problem in simultaneous Diophantine
approximation: Schmidt’s conjecture,” arXiv:1001.2694, 2010.

[14] P. M. Gruber and C. G. Lekkerkerker, Geometry of Numbers, 2nd ed., North-Holland, 1987.

[15] T. W. Cusick and M. E. Flahive, The Markoff and Lagrange Spectra, American Mathematical
Society, 1989.

[16] S. Korsky, “Affine Copies of Three-Point Patterns in Sets of Integers,” arXiv:2609.02308,
2026.
