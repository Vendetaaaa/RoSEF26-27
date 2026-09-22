import secrets
from hashlib import sha256


class EllipticCurve:
    def __init__(self, p, a, b, Gx, Gy, q):
        self.p = p
        self.a = a
        self.b = b
        self.Gx = Gx
        self.Gy = Gy
        self.G = (Gx, Gy)
        self.q = q

    def is_on_curve(self, P):
        if P is None:
            return True
        x, y = P
        return (y**2 - (x**3 + self.a * x + self.b)) % self.p == 0

    def point_add(self, P1, P2):
        if P1 is None:
            return P2
        if P2 is None:
            return P1

        x1, y1 = P1
        x2, y2 = P2

        if x1 == x2 and (y1 != y2 or y1 == 0):
            return None

        if x1 == x2 and y1 == y2:
            num = (3 * x1 * x1 + self.a) % self.p
            den = pow(2 * y1, -1, self.p)
        else:
            num = (y2 - y1) % self.p
            den = pow(x2 - x1, -1, self.p)

        m = (num * den) % self.p
        x3 = (m * m - x1 - x2) % self.p
        y3 = (m * (x1 - x3) - y1) % self.p

        return (x3, y3)

    def scalar_mult(self, k, P):
        R = None
        Q = P
        k = k % self.q

        while k > 0:
            if k & 1:
                R = self.point_add(R, Q)
            Q = self.point_add(Q, Q)
            k >>= 1

        return R


curve = EllipticCurve(
    p=0xFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFEFFFFFC2F,
    a=0,
    b=7,
    Gx=0x79BE667EF9DCBBAC55A06295CE870B07029BFCDB2DCE28D959F2815B16F81798,
    Gy=0x483ADA7726A3C4655DA4FBFC0E1108A8FD17B448A68554199C47D08FFB10D4B8,
    q=0xFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFEBAAEDCE6AF48A03BBFD25E8CD0364141,
)


def generate_keypair():
    d = secrets.randbelow(curve.q - 1) + 1
    Q = curve.scalar_mult(d, curve.G)
    return d, Q


def hash_message(message: bytes) -> int:
    digest = sha256(message).digest()
    return int.from_bytes(digest, "big") % curve.q


def sign(d: int, msg_hash: int, k: int = None) -> tuple[int, int, int]:
    if not 1 <= d < curve.q:
        raise ValueError("Private key must be in [1, q-1].")
    if not 0 <= msg_hash < curve.q:
        raise ValueError("Message hash must be in [0, q-1].")
    if k is not None and not 1 <= k < curve.q:
        raise ValueError("Nonce k must be in [1, q-1].")
    if k is None:
        k = secrets.randbelow(curve.q - 1) + 1

    R = curve.scalar_mult(k, curve.G)
    rx, _ = R

    r = rx % curve.q
    if r == 0:
        return sign(d, msg_hash)

    k_inv = pow(k, -1, curve.q)
    s = (k_inv * (msg_hash + r * d)) % curve.q
    if s == 0:
        return sign(d, msg_hash)

    return r, s, k


def verify(Q: tuple[int, int], msg_hash: int, r: int, s: int) -> bool:
    if not (1 <= r < curve.q and 1 <= s < curve.q):
        return False

    s_inv = pow(s, -1, curve.q)
    u1 = (s_inv * msg_hash) % curve.q
    u2 = (s_inv * r) % curve.q

    P1 = curve.scalar_mult(u1, curve.G)
    P2 = curve.scalar_mult(u2, Q)
    P = curve.point_add(P1, P2)

    if P is None:
        return False

    x_p, _ = P
    return (x_p % curve.q) == r


def recover_key_from_nonce_reuse(
    z1: int, sig1: tuple[int, int], z2: int, sig2: tuple[int, int]
) -> int:
    r1, s1 = sig1
    r2, s2 = sig2

    if r1 != r2:
        raise ValueError("Signatures must share the same r value for k-reuse recovery.")
    if s1 == s2:
        raise ValueError("Cannot recover a nonce when s1 == s2 modulo q.")

    num = (z1 - z2) % curve.q
    den = pow((s1 - s2) % curve.q, -1, curve.q)
    k_recovered = (num * den) % curve.q

    r_inv = pow(r1, -1, curve.q)
    d_recovered = (r_inv * (k_recovered * s1 - z1)) % curve.q

    return d_recovered


if __name__ == "__main__":
    print("=" * 65)
    print("                PURE PYTHON ECDSA SIMULATOR                    ")
    print("=" * 65)

    d, Q = generate_keypair()
    print("\n--- 1. Key Generation ---")
    print(f"Private Key (d)   : {hex(d)}")
    print(f"Public Key Q (x)  : {hex(Q[0])}")
    print(f"Public Key Q (y)  : {hex(Q[1])}")

    message1 = b"Transaction 1042: Transfer 5.0 BTC to Alice"
    z1 = hash_message(message1)
    r1, s1, k1 = sign(d, z1)

    print("\n--- 2. Signing Message 1 ---")
    print(f"Message           : {message1.decode()}")
    print(f"Message Hash (z1) : {hex(z1)}")
    print(f"Random Nonce (k1) : {hex(k1)}")
    print(f"Signature r1      : {hex(r1)}")
    print(f"Signature s1      : {hex(s1)}")

    is_valid1 = verify(Q, z1, r1, s1)
    print("\n--- 3. Verifying Signature 1 ---")
    print(f"Verification Result : {is_valid1} (Expected: True)")

    tampered_hash = hash_message(b"Transaction 1042: Transfer 500 BTC to Alice")
    is_valid_tampered = verify(Q, tampered_hash, r1, s1)
    print(f"Tampered Verification: {is_valid_tampered} (Expected: False)")

    print("\n--- 4. Demonstrating Nonce Reuse Vulnerability ---")
    message2 = b"Transaction 1043: Transfer 0.5 BTC to Bob"
    z2 = hash_message(message2)

    r2, s2, _ = sign(d, z2, k=k1)

    print(f"Message 2 Hash (z2) : {hex(z2)}")
    print(f"Reused Nonce (k)   : {hex(k1)}")
    print(f"Signature r2      : {hex(r2)} (Matches r1: {r1 == r2})")
    print(f"Signature s2      : {hex(s2)}")

    d_recovered = recover_key_from_nonce_reuse(z1, (r1, s1), z2, (r2, s2))

    print("\n--- 5. Cryptanalytic Key Recovery ---")
    print(f"Recovered PrivKey : {hex(d_recovered)}")
    print(f"Original PrivKey  : {hex(d)}")
    print(f"Keys Match       : {d == d_recovered}")
    print("=" * 65)
