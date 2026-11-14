#include <unity.h>
#include "lx200_coords.h"

// Attention : "\xDF" suivi d'un chiffre serait lu comme un seul caractère
// hexadécimal ("\xDF01"). D'où les chaînes coupées en deux : "\xDF" "01".

void setUp() {}
void tearDown() {}

static const double EPS = 1e-9;

static void test_parse_ra_long() {
    double h;
    TEST_ASSERT_TRUE(lx200::parseRA("05:35:17", h));
    TEST_ASSERT_DOUBLE_WITHIN(EPS, 5 + 35 / 60.0 + 17 / 3600.0, h);
}

static void test_parse_ra_short() {
    double h;
    TEST_ASSERT_TRUE(lx200::parseRA("05:35.3", h));
    TEST_ASSERT_DOUBLE_WITHIN(EPS, 5 + 35.3 / 60.0, h);
}

static void test_parse_ra_leading_space_and_decimals() {
    double h;
    TEST_ASSERT_TRUE(lx200::parseRA(" 05:35:17.25", h));
    TEST_ASSERT_DOUBLE_WITHIN(EPS, 5 + 35 / 60.0 + 17.25 / 3600.0, h);
}

static void test_parse_ra_rejects() {
    double h = 42;
    TEST_ASSERT_FALSE(lx200::parseRA("24:00:00", h));
    TEST_ASSERT_FALSE(lx200::parseRA("05:60:00", h));
    TEST_ASSERT_FALSE(lx200::parseRA("05:35:60", h));
    TEST_ASSERT_FALSE(lx200::parseRA("-05:00:00", h));
    TEST_ASSERT_FALSE(lx200::parseRA("05", h));
    TEST_ASSERT_FALSE(lx200::parseRA("abc", h));
    TEST_ASSERT_FALSE(lx200::parseRA("", h));
    TEST_ASSERT_FALSE(lx200::parseRA("05:-3:00", h));
    TEST_ASSERT_EQUAL_DOUBLE(42, h);  // valeur intacte en cas d'échec
}

static void test_parse_dec_stellarium_degree_byte() {
    double d;
    TEST_ASSERT_TRUE(lx200::parseDec("+22\xDF" "01:30", d));
    TEST_ASSERT_DOUBLE_WITHIN(EPS, 22 + 1 / 60.0 + 30 / 3600.0, d);
}

static void test_parse_dec_short_and_star() {
    double d;
    TEST_ASSERT_TRUE(lx200::parseDec("+22*01", d));
    TEST_ASSERT_DOUBLE_WITHIN(EPS, 22 + 1 / 60.0, d);
    TEST_ASSERT_TRUE(lx200::parseDec(" -45*30:00", d));
    TEST_ASSERT_DOUBLE_WITHIN(EPS, -45.5, d);
}

static void test_parse_dec_negative_zero_degrees() {
    double d;
    TEST_ASSERT_TRUE(lx200::parseDec("-00*30:00", d));
    TEST_ASSERT_DOUBLE_WITHIN(EPS, -0.5, d);
}

static void test_parse_dec_rejects() {
    double d;
    TEST_ASSERT_FALSE(lx200::parseDec("+91*00", d));
    TEST_ASSERT_FALSE(lx200::parseDec("+45*61", d));
    TEST_ASSERT_TRUE(lx200::parseDec("+90*00:00", d));
}

static void test_format_ra() {
    char out[16];
    lx200::formatRA(5 + 35 / 60.0 + 17 / 3600.0, true, out, sizeof(out));
    TEST_ASSERT_EQUAL_STRING("05:35:17", out);
    lx200::formatRA(5 + 35.3 / 60.0, false, out, sizeof(out));
    TEST_ASSERT_EQUAL_STRING("05:35.3", out);
}

static void test_format_ra_rounding_and_wrap() {
    char out[16];
    lx200::formatRA(24 - 0.4 / 3600.0, true, out, sizeof(out));  // 23:59:59,6
    TEST_ASSERT_EQUAL_STRING("00:00:00", out);
    lx200::formatRA(-1, true, out, sizeof(out));
    TEST_ASSERT_EQUAL_STRING("23:00:00", out);
    lx200::formatRA(12 + 59.96 / 60.0, false, out, sizeof(out));  // 12:59,96
    TEST_ASSERT_EQUAL_STRING("13:00.0", out);
}

static void test_format_dec() {
    char out[16];
    lx200::formatDec(22 + 1 / 60.0 + 30 / 3600.0, true, out, sizeof(out));
    TEST_ASSERT_EQUAL_STRING("+22\xDF" "01'30", out);
    lx200::formatDec(-0.5, true, out, sizeof(out));
    TEST_ASSERT_EQUAL_STRING("-00\xDF" "30'00", out);
    lx200::formatDec(-0.00001, true, out, sizeof(out));
    TEST_ASSERT_EQUAL_STRING("+00\xDF" "00'00", out);
    lx200::formatDec(-45.5, false, out, sizeof(out));
    TEST_ASSERT_EQUAL_STRING("-45\xDF" "30", out);
    lx200::formatDec(95, true, out, sizeof(out));
    TEST_ASSERT_EQUAL_STRING("+90\xDF" "00'00", out);
}

static void test_format_site() {
    char out[16];
    lx200::formatLatitude(48.89, out, sizeof(out));
    TEST_ASSERT_EQUAL_STRING("+48\xDF" "53", out);
    lx200::formatLongitude(2.35, out, sizeof(out));  // est → négatif en LX200
    TEST_ASSERT_EQUAL_STRING("-002\xDF" "21", out);
    lx200::formatLongitude(-71.5, out, sizeof(out));  // ouest → positif
    TEST_ASSERT_EQUAL_STRING("+071\xDF" "30", out);
}

static void test_round_trip() {
    char out[16];
    double back;
    for (double h = 0; h < 24; h += 0.37) {
        lx200::formatRA(h, true, out, sizeof(out));
        TEST_ASSERT_TRUE(lx200::parseRA(out, back));
        TEST_ASSERT_DOUBLE_WITHIN(0.5 / 3600.0 + EPS, h, back);
    }
    for (double d = -90; d <= 90; d += 3.7) {
        lx200::formatDec(d, true, out, sizeof(out));
        TEST_ASSERT_TRUE(lx200::parseDec(out, back));
        TEST_ASSERT_DOUBLE_WITHIN(0.5 / 3600.0 + EPS, d, back);
    }
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_parse_ra_long);
    RUN_TEST(test_parse_ra_short);
    RUN_TEST(test_parse_ra_leading_space_and_decimals);
    RUN_TEST(test_parse_ra_rejects);
    RUN_TEST(test_parse_dec_stellarium_degree_byte);
    RUN_TEST(test_parse_dec_short_and_star);
    RUN_TEST(test_parse_dec_negative_zero_degrees);
    RUN_TEST(test_parse_dec_rejects);
    RUN_TEST(test_format_ra);
    RUN_TEST(test_format_ra_rounding_and_wrap);
    RUN_TEST(test_format_dec);
    RUN_TEST(test_format_site);
    RUN_TEST(test_round_trip);
    return UNITY_END();
}
