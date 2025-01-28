#ifndef CJSON_PORT_H
#define CJSON_PORT_H

#ifdef __cplusplus
extern "C" {
#endif

/* libc dependent function */
double fabs(double x);
double strtod(const char *nptr,char **endptr);
int sscanf(const char *s, const char *fmt, ...);

#ifdef __cplusplus
} /* extern "C" { */
#endif

#endif /* CJSON_PORT_H */
