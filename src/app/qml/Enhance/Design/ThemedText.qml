// Texte du design system : style typographique de Theme.typo, interligne fixe, rendu natif.
import QtQuick

Text {
    // Un des styles de Theme.typo (display, title1, headline, body, callout, caption...).
    property QtObject textStyle: Theme.typo.body

    font: textStyle.font
    lineHeightMode: Text.FixedHeight
    lineHeight: textStyle.lineHeight
    renderType: Theme.textRenderType
    color: Theme.color.textPrimary
    elide: Text.ElideNone
}
