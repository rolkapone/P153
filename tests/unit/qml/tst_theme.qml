// Theme.qml (copie de docs/design/ui/Theme.qml) doit rester identique a tokens.json.
import QtQuick
import QtTest
import Enhance.Design

TestCase {
    name: "Theme"

    function cleanup() {
        Theme.reduceMotion = false;
        Theme.dark = true;
        Theme.highContrast = false;
    }

    function test_jetonsPresents() {
        verify(tokens !== undefined && tokens !== null, "tokens.json introuvable");
    }

    function test_couleurs() {
        verify(Qt.colorEqual(Theme.darkPalette.bgWindow, tokens.color.dark.bgWindow));
        verify(Qt.colorEqual(Theme.darkPalette.textSecondary, tokens.color.dark.textSecondary));
        verify(Qt.colorEqual(Theme.darkPalette.focusRing, tokens.color.dark.focusRing));
        verify(Qt.colorEqual(Theme.lightPalette.bgWindow, tokens.color.light.bgWindow));
        verify(Qt.colorEqual(Theme.lightPalette.focusRing, tokens.color.light.focusRing));
        verify(Qt.colorEqual(Theme.lightPalette.textPrimary, tokens.color.light.textPrimary));
    }

    function test_espacementsRayonsTailles() {
        compare(Theme.space.s2, tokens.space.s2);
        compare(Theme.space.s6, tokens.space.s6);
        compare(Theme.radius.xl, tokens.radius.xl);
        compare(Theme.size.toolbarH, tokens.size.toolbarH);
        compare(Theme.size.statusBarH, tokens.size.statusBarH);
        compare(Theme.size.windowMinW, tokens.size.windowMinW);
        compare(Theme.size.windowMinH, tokens.size.windowMinH);
        compare(Theme.size.windowDefaultW, tokens.size.windowDefaultW);
        compare(Theme.size.glassButtonH, tokens.size.glassButtonH);
        compare(Theme.size.glassButtonPadX, tokens.size.glassButtonPadX);
        compare(Theme.size.dropZoneBorder, tokens.size.dropZoneBorder);
    }

    function test_typographie() {
        compare(Theme.typo.title1.size, tokens.typography.style.title1.size);
        compare(Theme.typo.title1.lineHeight, tokens.typography.style.title1.lineHeight);
        compare(Theme.typo.caption.lineHeight, tokens.typography.style.caption.lineHeight);
        compare(Theme.typo.headline.size, tokens.typography.style.headline.size);
    }

    function test_mouvement() {
        compare(Theme.dur.themeSwap, tokens.motion.duration.themeSwap);
        compare(Theme.dur.windowAppear, tokens.motion.duration.windowAppear);
        compare(Theme.dur.medium, tokens.motion.duration.medium);
        const spring = tokens.motion.easing.spring;
        for (let i = 0; i < 4; ++i)
            fuzzyCompare(Theme.ease.spring[i], spring[i], 1e-9);
    }

    function test_verre() {
        compare(Theme.glass.regular.blur, tokens.glass.levels.regular.blur);
        compare(Theme.glass.regular.bezel, tokens.glass.levels.regular.bezel);
        compare(Theme.glass.regular.refraction, tokens.glass.levels.regular.refraction);
        compare(Theme.elevation.glassFar.y, tokens.elevation.glassFar.y);
        compare(Theme.elevation.glassFar.blur, tokens.elevation.glassFar.blur);
        compare(Theme.elevation.glassNear.y, tokens.elevation.glassNear.y);
        compare(Theme.elevation.glassNear.blur, tokens.elevation.glassNear.blur);
        // Biseau effectif de la capsule de 44 px (libelle headline 20) : 12 px.
        compare(Theme.glassBezelFor(Theme.glass.regular, Theme.size.glassButtonH,
                                    Theme.typo.headline.lineHeight), 12);
    }

    function test_mouvementReduit() {
        Theme.reduceMotion = true;
        compare(Theme.dur.moveMedium, 0);
        compare(Theme.dur.movePress, 0);
        compare(Theme.dur.themeSwap, 0);
        compare(Theme.dur.windowAppear, 0);
        compare(Theme.dur.fast, tokens.motion.duration.fast); // les fondus restent
        verify(!Theme.glassMotion);
    }

    function test_contrasteEleveSansMatiere() {
        Theme.highContrast = true;
        verify(!Theme.materialsEnabled);
        compare(Theme.glassQuality, Theme.glassOpaque);
        compare(Theme.borderWidth, 2);
    }
}
