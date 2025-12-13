#ifndef LANG_H
#define LANG_H

// =========================================================================
// LANGUAGE SELECTION - Uncomment ONE language
// =========================================================================
// #define LANG_CN  // 中文
// #define LANG_DE  // Deutsch
// #define LANG_EN  // English
// #define LANG_FR  // Français
// #define LANG_IT  // Italiano
// #define LANG_PT  // Português

#if !defined(LANG_CN) && !defined(LANG_DE) && !defined(LANG_EN) && !defined(LANG_ES) && !defined(LANG_FR) && !defined(LANG_IT) && !defined(LANG_PT)
#define LANG_ES  // Español por defecto
#endif

#if defined(LANG_CN)
  #include "lang_cn.h"
#elif defined(LANG_DE)
  #include "lang_de.h"
#elif defined(LANG_EN)
  #include "lang_en.h"
#elif defined(LANG_ES)
  #include "lang_es.h"
#elif defined(LANG_FR)
  #include "lang_fr.h"
#elif defined(LANG_IT)
  #include "lang_it.h"
#elif defined(LANG_PT)
  #include "lang_pt.h"
#endif

#define _(key) LANG_##key

#endif
