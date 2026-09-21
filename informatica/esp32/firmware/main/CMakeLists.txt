idf_component_register(
    SRCS "main.c" "ecdsa_exp.c"
    INCLUDE_DIRS "."
    REQUIRES mbedtls driver
)
