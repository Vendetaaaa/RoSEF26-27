# Verificarea cazurilor k = 4 și k = 5

Fișierul curent extinde verificarea computațională a părții matematice pentru familii cu patru și cinci drepte.

Scopul este să verificăm dacă relațiile construite pentru cazul scalar cu trei familii se comportă coerent atunci când numărul familiilor crește. Rezultatele sunt numerice. Ele nu constituie o demonstrație a unui transfer de exponent pentru aproximarea diofantină simultană în cazul general k > 3.

## Starea matematică actuală

Partea demonstrată riguros tratează cazul scalar cu trei familii, unde concurența se reduce la o singură relație de aproximare diofantină.

Pentru k > 3, alegem o dreaptă de referință și definim, pentru i < k:

Rᵢ = (κᵢ − κₖ) / (κ₂ − κₖ)

q = m₂ − mₖ

pᵢ = mᵢ − mₖ

Condiția de concurență devine:

pᵢ = Rᵢq.

Pentru k = 4, avem două relații simultane:

p₁ = R₁q

p₃ = R₃q

Pentru k = 5, avem trei relații simultane:

p₁ = R₁q

p₃ = R₃q

p₄ = R₄q

Prin urmare, problema trece de la un singur defect scalar la un vector de defecte.

Definim:

Δᵢ(pᵢ, q) = pᵢ − Rᵢq.

Pentru un sistem cu k familii, putem considera defectul comun:

Δ⁽ᵏ⁾(p, q) = maxᵢ₍ᵢ<k₎ |Δᵢ(pᵢ, q)|.

Concurența exactă corespunde cazului:

Δ⁽ᵏ⁾(p, q) = 0.

În cazul irațional, experimentul caută valori întregi pentru care defectul comun devine mic într-un domeniu finit.

---

# 1. Cazul k = 4

Considerăm patru familii de drepte:

x − κᵢy = mᵢ, unde i ∈ {1, 2, 3, 4}.

Alegem familia 4 drept referință și eliminăm x. Pentru i = 1, 2, 3 obținem:

mᵢ − m₄ = (κᵢ − κ₄)y.

Luăm familia 2 ca normalizare:

q = m₂ − m₄.

Pentru celelalte familii:

p₁ = m₁ − m₄

p₃ = m₃ − m₄

Definim:

R₁ = (κ₁ − κ₄) / (κ₂ − κ₄)

R₃ = (κ₃ − κ₄) / (κ₂ − κ₄)

Atunci sistemul devine:

p₁ = R₁q

p₃ = R₃q.

Defectele individuale sunt:

Δ₁ = p₁ − R₁q

Δ₃ = p₃ − R₃q

iar defectul comun este:

Δ⁽⁴⁾ = max{|Δ₁|, |Δ₃|}.

## 1.1 Ce verificăm numeric

Experimentul pentru k = 4 trebuie să verifice separat:

1. construcția coeficienților R₁ și R₃;
2. generarea valorilor întregi mᵢ;
3. eliminarea lui x;
4. corespondența dintre intersecția geometrică și relațiile aritmetice;
5. existența soluțiilor construite intenționat cu concurență;
6. comportamentul unui sistem fără constrângerea de concurență;
7. valoarea defectului comun pentru fiecare configurație.

Pentru un sistem construit astfel încât cele patru drepte să fie concurente, trebuie să obținem:

Δ₁ = 0

Δ₃ = 0.

Într-un sistem generic, experimentul nu presupune această egalitate. Se măsoară defectul obținut din valorile generate.

## 1.2 Verificarea construcției

Verificarea trebuie să compare două descrieri ale aceleiași configurații:

geometrie ↔ relații aritmetice.

Pentru fiecare soluție candidat se verifică dacă punctul obținut prin intersecția dreptei de referință cu celelalte drepte satisface toate ecuațiile.

În sens invers, pentru fiecare soluție aritmetică validă se verifică dacă valorile rezultate pentru x și y definesc aceeași intersecție geometrică.

Rezultatul trebuie raportat ca număr de soluții păstrate, număr de soluții respinse și, pentru cazurile concurente, diferența dintre cele două mulțimi.

---

# 2. Cazul k = 5

Considerăm cinci familii:

x − κᵢy = mᵢ, unde i ∈ {1, 2, 3, 4, 5}.

Alegem familia 5 drept referință:

q = m₂ − m₅.

Definim:

p₁ = m₁ − m₅

p₃ = m₃ − m₅

p₄ = m₄ − m₅

Coeficienții normalizați sunt:

R₁ = (κ₁ − κ₅) / (κ₂ − κ₅)

R₃ = (κ₃ − κ₅) / (κ₂ − κ₅)

R₄ = (κ₄ − κ₅) / (κ₂ − κ₅)

Condiția de concurență devine:

p₁ = R₁q

p₃ = R₃q

p₄ = R₄q.

Defectele individuale sunt:

Δ₁ = p₁ − R₁q

Δ₃ = p₃ − R₃q

Δ₄ = p₄ − R₄q.

Defectul comun poate fi măsurat prin:

Δ⁽⁵⁾ = max{|Δ₁|, |Δ₃|, |Δ₄|}.

Pentru concurență exactă:

Δ⁽⁵⁾ = 0.

## 2.1 Ce verificăm numeric

Experimentul pentru k = 5 trebuie să urmărească aceeași structură ca experimentul pentru k = 4, dar cu trei relații simultane:

- construcția sistemului;
- eliminarea lui x;
- calculul celor trei rapoarte Rᵢ;
- calculul defectelor;
- verificarea intersecției comune;
- comparația dintre soluțiile geometrice și cele aritmetice.

Creșterea lui k introduce o condiție suplimentară care trebuie satisfăcută de aceeași valoare q.

---

# 3. Domeniu finit

Pentru comparația numerică, păstrăm un domeniu finit pentru valorile întregi.

În analogie cu cazul scalar, putem considera:

Bₘ = [−M, M]ᵏ ∩ ℤᵏ.

Pentru fiecare candidat calculăm defectul comun.

Pentru k = 4:

Δ⁽⁴⁾ = max{|p₁ − R₁q|, |p₃ − R₃q|}.

Pentru k = 5:

Δ⁽⁵⁾ = max{|p₁ − R₁q|, |p₃ − R₃q|, |p₄ − R₄q|}.

Experimentul poate urmări:

δₘ⁽ᵏ⁾ = min Δ⁽ᵏ⁾

în domeniul finit ales.

Valorile δₘ⁽⁴⁾ și δₘ⁽⁵⁾ pot fi apoi comparate pentru mai multe valori ale lui M.

Această comparație este descriptivă. Ea nu stabilește prin ea însăși un exponent asimptotic.

---

# 4. Caz construit și caz generic

Pentru a separa verificarea algebrică de comportamentul generic, experimentul trebuie să păstreze două tipuri de sisteme.

## 4.1 Sistem construit

Valorile sunt alese astfel încât să existe o concurență cunoscută.

În acest caz verificăm dacă implementarea recuperează aceeași configurație prin:

ecuațiile drepte

și prin:

relațiile diofantice.

Pentru sistemul construit, defectul comun așteptat este:

Δ⁽ᵏ⁾ = 0.

## 4.2 Sistem generic

Coeficienții și valorile întregi sunt generate fără impunerea unei concurențe exacte.

În acest caz măsurăm:

Δ⁽ᵏ⁾ > 0

în general, iar experimentul urmărește distribuția valorilor obținute în domeniul finit.

Acest caz verifică faptul că implementarea nu produce concurență printr-o condiție introdusă accidental în construcție.

---

# 5. Comparația dintre k = 3, k = 4 și k = 5

Structura problemei poate fi rezumată astfel:

| caz | relații simultane | tipul defectului |
| --- | ---: | --- |
| k = 3 | 1 | scalar |
| k = 4 | 2 | vector cu 2 componente |
| k = 5 | 3 | vector cu 3 componente |

Pentru k = 3, relația se reduce la:

p = Rq.

Pentru k = 4 apar două aproximări simultane:

p₁ ≈ R₁q

p₃ ≈ R₃q.

Pentru k = 5 apar trei:

p₁ ≈ R₁q

p₃ ≈ R₃q

p₄ ≈ R₄q.

Această creștere arată unde începe problema de aproximare simultană.

---

# 6. Ce poate demonstra experimentul

Dacă verificările numerice trec, putem afirma că implementarea:

- construiește corect sistemele pentru k = 4 și k = 5;
- reproduce relațiile aritmetice rezultate din eliminarea lui x;
- identifică sistemele construite cu concurență;
- măsoară defectul comun pentru sistemele fără concurență exactă;
- păstrează corespondența dintre descrierea geometrică și cea aritmetică în cazurile testate.

Aceste rezultate verifică implementarea pe instanțele alese.

Ele nu oferă o demonstrație generală pentru k > 3.

---

# 7. Ce rămâne deschis

Pentru o extensie teoretică a rezultatului din cazul k = 3, trebuie definite riguros:

exponentul de aproximare simultană,

defectul comun,

și relația dintre:

δₘ⁽ᵏ⁾

și exponentul simultan asociat vectorului:

(R₁, ..., Rₖ₋₂).

Abia după stabilirea acestor definiții poate fi formulată și demonstrată o relație de transfer de exponent pentru k > 3.

Prin urmare, rezultatele pentru k = 4 și k = 5 trebuie prezentate ca verificări computaționale ale construcției, nu ca demonstrație a cazului general.

---

# 8. Reproductibilitate

Experimentele pentru k = 4 și k = 5 trebuie executate din folderul:

```text
matematica/
  experiments/
```

Rezultatele trebuie păstrate separat de demonstrația teoretică.

Pentru fiecare rulare trebuie înregistrate cel puțin:

- valoarea lui k;
- valoarea lui M;
- coeficienții κᵢ;
- valorile mᵢ;
- valorile Rᵢ;
- defectele individuale;
- defectul comun;
- numărul de configurații concurente;
- numărul de configurații respinse;
- seed-ul, dacă experimentul folosește aleatoriu.

Un rezultat `PASS` indică faptul că verificarea implementată a trecut testul definit de cod. Nu trebuie interpretat ca dovadă a unui rezultat teoretic care nu a fost demonstrat.

# Referințe

[1] J. W. S. Cassels, *An Introduction to Diophantine Approximation*, Cambridge University Press, 1957.

[2] J. W. S. Cassels, *An Introduction to the Geometry of Numbers*, Springer, 1959.

[3] G. H. Hardy and E. M. Wright, *An Introduction to the Theory of Numbers*, 6th ed., Oxford University Press, 2008.

[4] K. Ireland and M. Rosen, *A Classical Introduction to Modern Number Theory*, 2nd ed., Springer, 1990.

[5] A. Khintchine, “Einige Sätze über Kettenbrüche, mit Anwendungen auf die Theorie der Diophantischen Approximationen,” *Mathematische Annalen* 92 (1924), 115–125.

[6] W. M. Schmidt, *Diophantine Approximation*, Lecture Notes in Mathematics, vol. 785, Springer, 1980.

[7] G. Harman, *Metric Number Theory*, London Mathematical Society Monographs, New Series 18, Clarendon Press, 1998.

[8] V. G. Sprindzhuk, *Metric Theory of Diophantine Approximations*, V. H. Winston, 1979.

[9] Y. Bugeaud, *Approximation by Algebraic Numbers*, Cambridge University Press, 2004.

[10] K. F. Roth, “Rational approximations to algebraic numbers,” *Mathematika* 2 (1955), 1–20.

[11] H. Davenport and K. F. Roth, “Rational approximations to algebraic numbers,” *Mathematika* 2 (1955), 160–167.

[12] M. Waldschmidt, “Report on some recent advances in Diophantine approximation,” arXiv:0908.3973, 2009.

[13] D. Badziahin, A. Pollington, and S. Velani, “On a problem in simultaneous Diophantine approximation: Schmidt’s conjecture,” arXiv:1001.2694, 2010.

[14] P. M. Gruber and C. G. Lekkerkerker, *Geometry of Numbers*, 2nd ed., North-Holland, 1987.

[15] T. W. Cusick and M. E. Flahive, *The Markoff and Lagrange Spectra*, American Mathematical Society, 1989.

[16] S. Korsky, “Affine Copies of Three-Point Patterns in Sets of Integers,” arXiv:2609.02308, 2026.
