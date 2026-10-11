from dataset_generator3 import CURVE, Point, is_on_curve, scalar_mult


def verify_candidate(candidate_private_key: int, public_records: list[dict]) -> bool:
    """Return True only if candidate * G matches the dataset's public key."""
    if isinstance(candidate_private_key, bool) or not isinstance(candidate_private_key, int):
        return False
    if not 1 <= candidate_private_key < CURVE.n or not public_records:
        return False

    try:
        public_keys = []
        for record in public_records:
            key = record["public_key"]
            if not isinstance(key, dict) or key.get("curve") != "secp256k1":
                return False
            point = Point(int(key["x"], 16), int(key["y"], 16))
            if not is_on_curve(CURVE, point) or point.is_infinity:
                return False
            public_keys.append(point)
    except (AttributeError, KeyError, TypeError, ValueError):
        return False

    expected_public_key = public_keys[0]
    if any(point != expected_public_key for point in public_keys[1:]):
        return False

    candidate_public_key = scalar_mult(CURVE, candidate_private_key, CURVE.G)
    return candidate_public_key == expected_public_key
