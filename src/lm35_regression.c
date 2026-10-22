/*
 * lm35_regression.c
 *
 * Calibration check of an LM35 temperature sensor read by an Arduino Uno ADC.
 *
 * Input (standard input): lines of the form "set_temp_c,adc_reading".
 *   - A header line is skipped automatically.
 *   - Reading stops at a blank line or at end of input.
 *
 * What it does:
 *   1. Fits a straight line  adc = m * T + c  by least squares.
 *   2. Reports R^2 and the RMS error of the fit.
 *   3. Compares the fitted slope with the theoretical slope:
 *        LM35 gives 10 mV per degree C, and the 10-bit ADC has 5 V / 1024 per count,
 *        so the expected slope is 0.010 / (5 / 1024) = 2.048 counts per degree C.
 *   4. Converts each ADC reading back to degrees C using the formula in the
 *      Arduino code (adc * 5 * 100 / 1024) and reports the conversion error.
 *
 * Compile:  gcc lm35_regression.c -o lm35_regression -lm
 * Run:      ./lm35_regression < ../data/lm35_calibration.csv
 */

#include <stdio.h>
#include <math.h>

#define MAX_POINTS 200
#define VREF 5.0
#define ADC_STEPS 1024.0
#define LM35_VOLTS_PER_C 0.010

int main(void)
{
    double x[MAX_POINTS], y[MAX_POINTS];
    int n = 0;
    char line[128];

    /* ---- read the data ---- */
    while (n < MAX_POINTS && fgets(line, sizeof line, stdin) != NULL) {
        double t, adc;

        if (line[0] == '\n' || line[0] == '\r') {
            break;                          /* blank line ends the input */
        }
        if (sscanf(line, "%lf,%lf", &t, &adc) == 2) {
            x[n] = t;
            y[n] = adc;
            n++;
        }                                   /* lines that do not parse (the header) are skipped */
    }

    if (n < 2) {
        printf("Need at least 2 data points, found %d.\n", n);
        return 1;
    }

    /* ---- least-squares fit: y = m*x + c ---- */
    double sum_x = 0.0, sum_y = 0.0;
    for (int i = 0; i < n; i++) {
        sum_x += x[i];
        sum_y += y[i];
    }
    double mean_x = sum_x / n;
    double mean_y = sum_y / n;

    double sxx = 0.0, sxy = 0.0;
    for (int i = 0; i < n; i++) {
        sxx += (x[i] - mean_x) * (x[i] - mean_x);
        sxy += (x[i] - mean_x) * (y[i] - mean_y);
    }
    double m = sxy / sxx;
    double c = mean_y - m * mean_x;

    /* ---- goodness of fit ---- */
    double ss_res = 0.0, ss_tot = 0.0;
    for (int i = 0; i < n; i++) {
        double fit = m * x[i] + c;
        ss_res += (y[i] - fit) * (y[i] - fit);
        ss_tot += (y[i] - mean_y) * (y[i] - mean_y);
    }
    double r2 = 1.0 - ss_res / ss_tot;
    double rmse = sqrt(ss_res / n);

    /* ---- compare with theory ---- */
    double theory_slope = LM35_VOLTS_PER_C / (VREF / ADC_STEPS);
    double slope_diff_pct = 100.0 * (m - theory_slope) / theory_slope;

    printf("LM35 + Arduino ADC calibration check\n");
    printf("====================================\n");
    printf("Data points          : %d\n", n);
    printf("Fitted line          : ADC = %.4f * T + %.3f\n", m, c);
    printf("R-squared            : %.6f\n", r2);
    printf("RMS error of fit     : %.3f ADC counts\n", rmse);
    printf("Theoretical slope    : %.4f counts per degree C\n", theory_slope);
    printf("Slope difference     : %+.3f %%\n\n", slope_diff_pct);

    /* ---- per-point table and conversion error ---- */
    printf("%8s %6s %9s %9s %10s\n", "Set(C)", "ADC", "Fit", "Resid", "Conv(C)");
    double sum_sq_err = 0.0, max_abs_err = 0.0;
    for (int i = 0; i < n; i++) {
        double fit = m * x[i] + c;
        double conv = y[i] * VREF * 100.0 / ADC_STEPS;   /* same formula as the Arduino code */
        double err = conv - x[i];

        printf("%8.1f %6.0f %9.2f %9.2f %10.2f\n", x[i], y[i], fit, y[i] - fit, conv);
        sum_sq_err += err * err;
        if (fabs(err) > max_abs_err) {
            max_abs_err = fabs(err);
        }
    }
    printf("\nArduino conversion error: RMS %.3f C, maximum %.3f C\n",
           sqrt(sum_sq_err / n), max_abs_err);

    return 0;
}
