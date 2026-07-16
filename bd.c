#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sqlite3.h>
#include <math.h>

static double compute_variance(double *values, int count, double mean) {
    if (count <= 1) return 0.0;
    double sum_sq = 0.0;
    for (int i = 0; i < count; i++) {
        double diff = values[i] - mean;
        sum_sq += diff * diff;
    }
    return sum_sq / (count - 1);
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <database> <table> <column>\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char *db_path = argv[1];
    const char *table = argv[2];
    const char *column = argv[3];

    sqlite3 *db = NULL;
    char *errmsg = NULL;
    int rc;

    rc = sqlite3_open(db_path, &db);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
        return EXIT_FAILURE;
    }

    char check_table[512];
    snprintf(check_table, sizeof(check_table),
             "SELECT name FROM sqlite_master WHERE type='table' AND name='%s';", table);
    rc = sqlite3_exec(db, check_table, NULL, NULL, &errmsg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Table '%s' does not exist or error: %s\n", table, errmsg);
        sqlite3_free(errmsg);
        sqlite3_close(db);
        return EXIT_FAILURE;
    }

    char query[1024];
    snprintf(query, sizeof(query),
             "SELECT \"%s\" FROM \"%s\" WHERE \"%s\" IS NOT NULL;", column, table, column);

    sqlite3_stmt *stmt = NULL;
    rc = sqlite3_prepare_v2(db, query, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare query: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return EXIT_FAILURE;
    }

    double *values = NULL;
    int capacity = 0;
    int count = 0;

    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        int col_type = sqlite3_column_type(stmt, 0);
        if (col_type == SQLITE_INTEGER || col_type == SQLITE_FLOAT) {
            double val = sqlite3_column_double(stmt, 0);
            if (capacity == 0) {
                capacity = 64;
                values = malloc(capacity * sizeof(double));
                if (!values) {
                    fprintf(stderr, "Memory allocation failed\n");
                    sqlite3_finalize(stmt);
                    sqlite3_close(db);
                    return EXIT_FAILURE;
                }
            } else if (count >= capacity) {
                capacity *= 2;
                double *new_values = realloc(values, capacity * sizeof(double));
                if (!new_values) {
                    fprintf(stderr, "Memory reallocation failed\n");
                    free(values);
                    sqlite3_finalize(stmt);
                    sqlite3_close(db);
                    return EXIT_FAILURE;
                }
                values = new_values;
            }
            values[count++] = val;
        } else {
            fprintf(stderr, "Warning: non-numeric value in column '%s' ignored\n", column);
        }
    }

    sqlite3_finalize(stmt);

    if (count == 0) {
        printf("No numeric data found in column '%s'.\n", column);
        free(values);
        sqlite3_close(db);
        return EXIT_SUCCESS;
    }

    double sum = 0.0;
    double min = values[0];
    double max = values[0];
    for (int i = 0; i < count; i++) {
        sum += values[i];
        if (values[i] < min) min = values[i];
        if (values[i] > max) max = values[i];
    }
    double mean = sum / count;
    double variance = compute_variance(values, count, mean);

    printf("Statistics for column '%s' in table '%s':\n", column, table);
    printf("  Count (numeric): %d\n", count);
    printf("  Sum: %.6f\n", sum);
    printf("  Mean: %.6f\n", mean);
    printf("  Min: %.6f\n", min);
    printf("  Max: %.6f\n", max);
    printf("  Variance: %.6f\n", variance);

    free(values);
    sqlite3_close(db);
    return EXIT_SUCCESS;
}