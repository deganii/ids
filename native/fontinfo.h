#ifndef IDS_FONTINFO_H
#define IDS_FONTINFO_H

#ifdef __cplusplus
extern "C" {
#endif
	typedef struct {
		const short *CharacterMap;
		const int *GlyphAdvances;
		int Count;
		int descender_height;
		int font_height;
		VGPath Glyphs[500];
	} Fontinfo;

	extern Fontinfo SansTypeface, SerifTypeface, MonoTypeface;

#ifdef __cplusplus
}
#endif

#endif // IDS_FONTINFO_H
