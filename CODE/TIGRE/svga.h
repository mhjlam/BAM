#ifndef SVGAH
#define SVGAH

#ifdef __cplusplus
extern "C" {
#endif
  /* Only SVGASetPalette is used in the Linux port (FMV path, flicsmk.cpp).
   * All other DOS SVGA hardware functions have been removed. */
  void SVGASetPalette(void* pal);
#ifdef __cplusplus
}
#endif

#endif
