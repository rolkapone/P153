// Squelette ENH-004 : contenu strict, textes exacts (§5 de la note UX), grille, clavier,
// replis (mouvement reduit, contraste eleve, transparence coupee). Sans GPU.
import QtQuick
import QtTest
import Enhance.Design
import Enhance.App

TestCase {
    id: tc
    name: "Squelette"
    when: windowShown
    width: 1100
    height: 720

    Component {
        id: contentComponent
        AppContent {
            width: tc.width
            height: tc.height
            focus: true
        }
    }

    property Item content: null

    function init() {
        // TestCase est invisible : le contenu est pose directement dans la fenetre de test.
        content = createTemporaryObject(contentComponent, tc.Window.contentItem);
        verify(content !== null);
        content.forceActiveFocus();
    }

    function cleanup() {
        Theme.reduceMotion = false;
        Theme.highContrast = false;
        Theme.transparencyEnabled = true;
        Theme.dark = true;
    }

    function find(name) {
        const item = findChild(content, name);
        verify(item !== null, "objet introuvable : " + name);
        return item;
    }

    // Tous les Text visibles de l'arbre.
    function visibleTexts(item, out) {
        if (!item.visible)
            return out;
        if (item.lineHeightMode !== undefined && item.renderType !== undefined && typeof item.text === "string")
            out.push(item.text);
        for (let i = 0; i < item.children.length; ++i)
            visibleTexts(item.children[i], out);
        return out;
    }

    function test_textesExacts() {
        compare(find("emptyTitle").text, "Déposez vos vidéos ici");
        compare(find("emptySubtitle").text,
                "Enhance les passe en 4K fluide, sur votre ordinateur. Rien n'est envoyé sur Internet.");
        compare(find("springCapsule").text, "Tester le ressort");
        compare(find("springCounter").text, "Essais : " + DemoCounter.count);
        compare(find("springMention").text, "Démonstration technique");
        compare(find("statusText").text, "GeForce RTX 5070 · prête");
    }

    function test_contenuStrict() {
        const texts = visibleTexts(content, []);
        compare(texts.length, AppStatus.elevated ? 7 : 6, "textes visibles : " + JSON.stringify(texts));
    }

    function test_grilleA1100x720() {
        const frame = find("emptyState");
        compare(frame.y, Theme.size.toolbarH);
        compare(frame.height, 520);
        compare(frame.width, Theme.size.contentMaxW); // colonne 928 - 2 x 24
        const capsule = find("springCapsule");
        compare(capsule.height, Theme.size.glassButtonH);
        verify(capsule.width >= Theme.size.primaryButtonMinW);
        const demo = find("springDemo");
        compare(demo.y, frame.y + frame.height + Theme.space.s6);
        compare(find("statusBar").height, Theme.size.statusBarH);
        const counter = find("springCounter");
        compare(counter.x, capsule.x + capsule.width + Theme.space.s4);
        // Capsule centree dans la colonne, au pixel entier.
        const center = capsule.mapToItem(content, capsule.width / 2, 0).x;
        verify(Math.abs(center - content.width / 2) <= 0.5);
        compare(capsule.x, Math.round(capsule.x));
    }

    function test_grilleTailleMinimale() {
        content.width = Theme.size.windowMinW;
        content.height = Theme.size.windowMinH;
        const frame = find("emptyState");
        compare(frame.height, 320);
        compare(frame.x, Theme.space.s4); // marges de 16 sous 900 px
        compare(frame.width, Theme.size.windowMinW - 2 * Theme.space.s4);
        const subtitle = find("emptySubtitle");
        verify(subtitle.lineCount >= 2, "le sous-titre revient a la ligne");
        verify(!subtitle.truncated);
    }

    function test_clicAppelleLeCpp() {
        const before = DemoCounter.count;
        mouseClick(find("springCapsule"));
        compare(DemoCounter.count, before + 1);
        compare(find("springCounter").text, "Essais : " + (before + 1));
        verify(!find("springCapsule").visualFocus, "aucun anneau au clic souris");
    }

    function test_clavierSeul() {
        const capsule = find("springCapsule");
        verify(!capsule.activeFocus, "focus initial sur la fenetre, pas sur la capsule");
        keyClick(Qt.Key_Tab);
        verify(capsule.activeFocus);
        verify(capsule.visualFocus);
        tryCompare(find("focusRing"), "opacity", 1);
        compare(find("focusRing").border.width, Theme.size.focusRingW);

        const before = DemoCounter.count;
        keyClick(Qt.Key_Return);
        compare(DemoCounter.count, before + 1);
        keyClick(Qt.Key_Enter);
        compare(DemoCounter.count, before + 2);
        keyClick(Qt.Key_Space);
        compare(DemoCounter.count, before + 3);

        // Un seul arret de tabulation : Tab ne piege pas (il revient sur la capsule).
        keyClick(Qt.Key_Tab);
        verify(capsule.activeFocus);
    }

    function test_mouvementReduitAucuneEchelle() {
        Theme.reduceMotion = true;
        const capsule = find("springCapsule");
        const layer = find("glassLayer");
        mousePress(capsule);
        wait(Theme.dur.press * 2);
        compare(layer.scale, 1);
        mouseRelease(capsule);
        keyClick(Qt.Key_Tab);
        keyClick(Qt.Key_Return);
        wait(Theme.dur.medium);
        compare(layer.scale, 1);
    }

    function test_transparenceCoupeeVerreOpaque() {
        Theme.transparencyEnabled = false;
        tryCompare(find("glassOpaqueFace"), "opacity", 1);
        tryCompare(find("glassShader"), "visible", false);
    }

    function test_contrasteEleveOpaqueEtLisere() {
        Theme.highContrast = true;
        const face = find("glassOpaqueFace");
        tryCompare(face, "opacity", 1);
        compare(face.border.width, 2);
        tryCompare(find("glassShader"), "visible", false);
    }

    function test_aucuneZoneDeDepot() {
        // Depot inerte (UX-3) : aucune DropArea dans l'arbre.
        function hasDropArea(item) {
            if (item.containsDrag !== undefined && item.drag !== undefined && item.keys !== undefined)
                return true;
            for (let i = 0; i < item.children.length; ++i) {
                if (hasDropArea(item.children[i]))
                    return true;
            }
            return false;
        }
        verify(!hasDropArea(content));
    }
}
