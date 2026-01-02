#pragma once
#include <cstdio>

// ANSI color codes
#define COLOR_RESET "\033[0m"
#define COLOR_INFO "\033[32m"  // Green
#define COLOR_DEBUG "\033[36m" // Cyan
#define COLOR_ERROR "\033[31m" // Red

// Log macros
#define LOG_INFO(fmt, ...)                                                     \
  printf(COLOR_INFO "[INFO] " COLOR_RESET fmt "\n", ##__VA_ARGS__)

#define LOG_DEBUG(fmt, ...)                                                    \
  printf(COLOR_DEBUG "[DEBUG] " COLOR_RESET fmt "\n", ##__VA_ARGS__)

#define LOG_ERROR(fmt, ...)                                                    \
  fprintf(stderr, COLOR_ERROR "[ERROR] " COLOR_RESET fmt "\n", ##__VA_ARGS__)
