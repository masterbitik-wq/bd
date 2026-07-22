#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sqlite3.h>
#include <math.h>

#define FREE(p) do { free(p); (p) = NULL; } while (0)

static void usage(const char *prog)
{
    fprintf(stderr, "Usage: %s -d <database> -t <table> -c <column>\n", prog);
}

static double compute_variance(double *values, int count, double mean)
{
    if (count <= 1)
        return 0.0;

    double sum_sq = 0.0;
    for (int i = 0; i < count; i++) {
        double diff = values[i] - mean;
        sum_sq += diff * diff;
    }
    return sum_sq / (count - 1);
}

int main(int argc, char *argv[])
{
    const char *db_path = NULL;
    const char *table = NULL;
    const char *column = NULL;
    int opt;
    int exit_code = EXIT_FAILURE;

    sqlite3 *db = NULL;
    char *errmsg = NULL;
    sqlite3_stmt *stmt = NULL;
    double *values = NULL;
    int capacity = 0;
    int count = 0;
    int rc;

    while ((opt = getopt(argc, argv, "d:t:c:h")) != -1) {
        switch (opt) {
        case 'd':
            db_path = optarg;
            break;
        case 't':
            table = optarg;
            break;
        case 'c':
            column = optarg;
            break;
        case 'h':
        default:
            usage(argv[0]);
            exit(EXIT_FAILURE);
        }
    }

    if (db_path == NULL || table == NULL || column == NULL) {
        usage(argv[0]);
        exit(EXIT_FAILURE);
    }

    rc = sqlite3_open(db_path, &db);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
        goto cleanup;
    }

    {
        char check_table[512];
        snprintf(check_table, sizeof(check_table),
                 "SELECT name FROM sqlite_master WHERE type='table' AND name='%s';",
                 table);
        rc = sqlite3_exec(db, check_table, NULL, NULL, &errmsg);
        if (rc != SQLITE_OK) {
            fprintf(stderr, "Table '%s' does not exist or error: %s\n",
                    table, errmsg);
            goto cleanup;
        }
    }

    {
        char query[1024];
        snprintf(query, sizeof(query),
                 "SELECT \"%s\" FROM \"%s\" WHERE \"%s\" IS NOT NULL;",
                 column, table, column);

        rc = sqlite3_prepare_v2(db, query, -1, &stmt, NULL);
        if (rc != SQLITE_OK) {
            fprintf(stderr, "Failed to prepare query: %s\n", sqlite3_errmsg(db));
            goto cleanup;
        }
    }

    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        int col_type = sqlite3_column_type(stmt, 0);

        if (col_type == SQLITE_INTEGER || col_type == SQLITE_FLOAT) {
            double val = sqlite3_column_double(stmt, 0);

            if (capacity == 0) {
                capacity = 64;
                values = malloc(capacity * sizeof(double));
                if (values == NULL) {
                    fprintf(stderr, "Memory allocation failed\n");
                    goto cleanup;
                }
            } else if (count >= capacity) {
                capacity *= 2;
                double *new_values = realloc(values, capacity * sizeof(double));
                if (new_values == NULL) {
                    fprintf(stderr, "Memory reallocation failed\n");
                    goto cleanup;
                }
                values = new_values;
            }
            values[count++] = val;
        } else {
            fprintf(stderr,
                    "Warning: non-numeric value in column '%s' ignored\n",
                    column);
        }
    }

    if (count == 0) {
        printf("No numeric data found in column '%s'.\n", column);
        exit_code = EXIT_SUCCESS;
        goto cleanup;
    }

    {
        double sum = 0.0;
        double min = values[0];
        double max = values[0];

        for (int i = 0; i < count; i++) {
            sum += values[i];
            if (values[i] < min)
                min = values[i];
            if (values[i] > max)
                max = values[i];
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
    }

    exit_code = EXIT_SUCCESS;

cleanup:
    if (errmsg != NULL) {
        sqlite3_free(errmsg);
        errmsg = NULL;
    }
    if (stmt != NULL) {
        sqlite3_finalize(stmt);
        stmt = NULL;
    }
    FREE(values);
    if (db != NULL) {
        sqlite3_close(db);
        db = NULL;
    }
    exit(exit_code);
}
