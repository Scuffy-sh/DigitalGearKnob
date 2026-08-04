SET(SOURCES screens/ui_InterfazMarchas.c
    screens/ui_InterfazLogo.c
    ui.c
    components/ui_comp_hook.c
    ui_helpers.c
    images/ui_img_vwlogonegro466x466_png.c
    fonts/ui_font_FuenteMarchas.c
    fonts/ui_font_FuenteTexto.c
    fonts/ui_font_FuenteVW.c)

add_library(ui ${SOURCES})
