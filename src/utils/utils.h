/**
 * @file utils.h
 * @brief General utility functions
 *
 * This header provides a collection of utility functions used throughout the
 * weather client application. It includes URL encoding for HTTP requests,
 * validation functions for coordinates and city names, time utilities,
 * string manipulation functions, and cache key normalization.
 *
 * Key features:
 * - URL encoding (RFC 3986 compliant)
 * - Coordinate validation (latitude/longitude)
 * - City name validation
 * - High-precision timestamp generation
 * - String trimming and duplication
 * - String normalization for cache keys
 *
 * @note All functions that allocate memory (url_encode, string_trim,
 *       string_duplicate) return pointers that must be freed by the caller.
 */
#ifndef UTILS_H
#define UTILS_H

#include <stddef.h>
#include <stdint.h>
#include <jansson.h>

#define CITY_MAX_LEN 64
#define PRICE_CLASS_LEN 4

typedef struct {
    char city[CITY_MAX_LEN];
    char price_class[PRICE_CLASS_LEN];
} EnergyConfig;

int load_config(const char *filename, EnergyConfig *config);

int save_config(const char *filename, const EnergyConfig *config);

/**
 * @brief URL-encodes a string according to RFC 3986
 *
 * Converts a string into a URL-safe format by encoding special characters
 * as percent-encoded sequences (e.g., space becomes %20). This is essential
 * for safely including user input in HTTP query parameters.
 *
 * Characters that are NOT encoded (unreserved):
 * - Letters: A-Z, a-z
 * - Digits: 0-9
 * - Symbols: - _ . ~
 *
 * All other characters are encoded as %XX where XX is the hexadecimal
 * representation of the character's byte value.
 *
 * @param str The string to encode (must be null-terminated)
 *
 * @return A newly allocated URL-encoded string, or NULL if:
 *         - Input string is NULL
 *         - Memory allocation fails
 *
 * @warning The returned string is dynamically allocated and must be freed
 *          by the caller using free().
 *
 * @see validate_city_name()
 *
 * @par Example:
 * @code
 * const char *city = "New York";
 * char *encoded = url_encode(city);
 * if (encoded) {
 *     printf("Encoded: %s\n", encoded);  // Output: "New%20York"
 *     free(encoded);
 * }
 * @endcode
 *
 * @par Example with special characters:
 * @code
 * const char *query = "São Paulo";
 * char *encoded = url_encode(query);
 * if (encoded) {
 *     printf("%s\n", encoded);  // "S%C3%A3o%20Paulo"
 *     free(encoded);
 * }
 * @endcode
 */
char* url_encode(const char* str);

/**
 * @brief Validates a latitude coordinate
 *
 * Checks if the provided latitude value is within the valid range for
 * geographic coordinates. Valid latitudes range from -90° (South Pole)
 * to +90° (North Pole).
 *
 * @param lat The latitude value to validate (in decimal degrees)
 *
 * @return 1 if the latitude is valid (within [-90.0, +90.0]), 0 otherwise
 *
 * @note This function uses inclusive bounds, so -90.0 and +90.0 are valid.
 *
 * @see validate_longitude()
 *
 * @par Example:
 * @code
 * if (validate_latitude(59.33)) {
 *     printf("Valid latitude\n");
 * }
 *
 * if (!validate_latitude(100.0)) {
 *     fprintf(stderr, "Latitude out of range\n");
 * }
 * @endcode
 */
int validate_latitude(double lat);

/**
 * @brief Validates a longitude coordinate
 *
 * Checks if the provided longitude value is within the valid range for
 * geographic coordinates. Valid longitudes range from -180° (west of
 * Prime Meridian) to +180° (east of Prime Meridian).
 *
 * @param lon The longitude value to validate (in decimal degrees)
 *
 * @return 1 if the longitude is valid (within [-180.0, +180.0]), 0 otherwise
 *
 * @note This function uses inclusive bounds, so -180.0 and +180.0 are valid.
 *
 * @see validate_latitude()
 *
 * @par Example:
 * @code
 * if (validate_longitude(18.07)) {
 *     printf("Valid longitude\n");
 * }
 *
 * if (!validate_longitude(200.0)) {
 *     fprintf(stderr, "Longitude out of range\n");
 * }
 * @endcode
 */
int validate_longitude(double lon);

/**
 * @brief Validates a city name string
 *
 * Checks if the provided city name is valid for use in API requests.
 * A valid city name must:
 * - Not be NULL
 * - Not be empty (length > 0)
 * - Contain at least one non-whitespace character
 *
 * @param city The city name to validate (null-terminated string)
 *
 * @return 1 if the city name is valid, 0 otherwise
 *
 * @note This function does not validate the actual existence of the city,
 *       only that the string format is acceptable for querying.
 *
 * @see url_encode(), weather_client_get_weather_by_city()
 *
 * @par Example:
 * @code
 * if (validate_city_name("Stockholm")) {
 *     // Proceed with API request
 * }
 *
 * if (!validate_city_name("")) {
 *     fprintf(stderr, "Empty city name\n");
 * }
 *
 * if (!validate_city_name("   ")) {
 *     fprintf(stderr, "City name contains only whitespace\n");
 * }
 * @endcode
 */
int validate_city_name(const char* city);

/**
 * @brief Gets the current time in milliseconds since Unix epoch
 *
 * Returns a high-precision timestamp representing the number of milliseconds
 * that have elapsed since January 1, 1970 00:00:00 UTC (Unix epoch).
 * This is useful for measuring elapsed time, timestamping cache entries,
 * and implementing timeouts.
 *
 * @return Current time in milliseconds as a 64-bit unsigned integer
 *
 * @note The function uses clock_gettime(CLOCK_REALTIME) on Unix systems
 *       for nanosecond precision, which is then converted to milliseconds.
 *
 * @note The returned value is susceptible to system clock changes. For
 *       measuring durations unaffected by clock adjustments, consider using
 *       CLOCK_MONOTONIC instead (not provided by this function).
 *
 * @par Example:
 * @code
 * uint64_t start_time = get_current_time_ms();
 * // ... perform some operation ...
 * uint64_t end_time = get_current_time_ms();
 * uint64_t elapsed = end_time - start_time;
 * printf("Operation took %llu ms\n", (unsigned long long)elapsed);
 * @endcode
 *
 * @par Example with cache entry:
 * @code
 * typedef struct {
 *     char *data;
 *     uint64_t timestamp;
 *     int ttl_seconds;
 * } CacheEntry;
 *
 * CacheEntry entry;
 * entry.data = strdup("cached data");
 * entry.timestamp = get_current_time_ms();
 * entry.ttl_seconds = 300;  // 5 minutes
 *
 * // Later, check if expired
 * uint64_t now = get_current_time_ms();
 * uint64_t age_ms = now - entry.timestamp;
 * if (age_ms > entry.ttl_seconds * 1000) {
 *     printf("Cache entry expired\n");
 * }
 * @endcode
 */
uint64_t get_current_time_ms();

/**
 * @brief Trims leading and trailing whitespace from a string
 *
 * Creates a new string with leading and trailing whitespace characters
 * removed from the input string. The original string is not modified.
 * Whitespace characters include: space, tab, newline, carriage return,
 * form feed, and vertical tab (as defined by isspace()).
 *
 * @param str The string to trim (null-terminated)
 *
 * @return A newly allocated trimmed string, or NULL if:
 *         - Input string is NULL
 *         - Memory allocation fails
 *
 * @warning The returned string is dynamically allocated and must be freed
 *          by the caller using free().
 *
 * @note If the input string contains only whitespace, the function returns
 *       an empty string (not NULL).
 *
 * @see string_duplicate(), normalize_string_for_cache()
 *
 * @par Example:
 * @code
 * char *trimmed = string_trim("  Stockholm  ");
 * if (trimmed) {
 *     printf("'%s'\n", trimmed);  // Output: 'Stockholm'
 *     free(trimmed);
 * }
 * @endcode
 *
 * @par Example with empty result:
 * @code
 * char *trimmed = string_trim("    ");
 * if (trimmed) {
 *     printf("Length: %zu\n", strlen(trimmed));  // Output: Length: 0
 *     free(trimmed);
 * }
 * @endcode
 */
char* string_trim(char* str);

/**
 * @brief Duplicates a string
 *
 * Creates a new dynamically allocated copy of the input string. This is
 * a safe wrapper around strdup() that works across different platforms.
 *
 * @param str The string to duplicate (null-terminated)
 *
 * @return A newly allocated copy of the string, or NULL if:
 *         - Input string is NULL
 *         - Memory allocation fails
 *
 * @warning The returned string is dynamically allocated and must be freed
 *          by the caller using free().
 *
 * @see string_trim()
 *
 * @par Example:
 * @code
 * const char *original = "Stockholm";
 * char *copy = string_duplicate(original);
 * if (copy) {
 *     printf("Copy: %s\n", copy);
 *     free(copy);
 * }
 * @endcode
 *
 * @par Example in structure:
 * @code
 * typedef struct {
 *     char *city_name;
 * } Location;
 *
 * Location loc;
 * loc.city_name = string_duplicate("Stockholm");
 * if (!loc.city_name) {
 *     fprintf(stderr, "Failed to duplicate city name\n");
 *     return -1;
 * }
 * // Later...
 * free(loc.city_name);
 * @endcode
 */
char* string_duplicate(const char* str);

/**
 * @brief Normalizes a string for use as a cache key
 *
 * Converts a string to a normalized form suitable for use in cache keys.
 * The normalization process ensures that semantically equivalent inputs
 * (e.g., with different whitespace or case) produce the same cache key.
 *
 * Normalization steps:
 * 1. Convert to lowercase
 * 2. Trim leading and trailing whitespace
 * 3. Replace internal whitespace sequences with a single space
 *
 * This ensures cache hits for queries like "New York", "new york",
 * "NEW  YORK" all map to the same cache entry.
 *
 * @param in The input string to normalize (null-terminated)
 * @param out Buffer to store the normalized string
 * @param out_size Size of the output buffer (must be > 0)
 *
 * @note The output buffer must be large enough to hold the normalized
 *       string plus the null terminator. If the input is NULL or empty,
 *       the output will be an empty string.
 *
 * @note The output is guaranteed to be null-terminated if out_size > 0,
 *       even if truncation occurs.
 *
 * @warning If the normalized string is longer than out_size-1, it will
 *          be truncated to fit. No error indication is provided.
 *
 * @see build_cache_key(), client_cache_set()
 *
 * @par Example:
 * @code
 * char normalized[256];
 * normalize_string_for_cache("  New   York  ", normalized, sizeof(normalized));
 * printf("'%s'\n", normalized);  // Output: 'new york'
 * @endcode
 *
 * @par Example in cache key building:
 * @code
 * char normalized_city[256];
 * char normalized_country[256];
 * normalize_string_for_cache(city, normalized_city, sizeof(normalized_city));
 * normalize_string_for_cache(country, normalized_country,
 *                           sizeof(normalized_country));
 *
 * char cache_key[512];
 * snprintf(cache_key, sizeof(cache_key), "weather:%s:%s",
 *          normalized_city, normalized_country);
 * // "weather:new york:us" and "weather:New York:US" produce the same key
 * @endcode
 */

void normalize_string_for_cache(const char* in, char* out, size_t out_size);

#endif
