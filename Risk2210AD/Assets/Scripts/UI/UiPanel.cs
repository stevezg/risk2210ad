using UnityEngine;
using UnityEngine.UIElements;

namespace Risk2210.UI
{
    /// <summary>Creates a runtime UIDocument with our theme and stylesheets (no asset authoring required).</summary>
    public static class UiPanel
    {
        private static PanelSettings settings;

        public static UIDocument Create(GameObject host, string uxmlResource, params string[] ussResources)
        {
            if (settings == null)
            {
                settings = ScriptableObject.CreateInstance<PanelSettings>();
                settings.themeStyleSheet = Resources.Load<ThemeStyleSheet>("UnityDefaultRuntimeTheme");
                settings.scaleMode = PanelScaleMode.ScaleWithScreenSize;
                settings.referenceResolution = new Vector2Int(1920, 1080);
                settings.match = 0.5f;
                settings.sortingOrder = 10;
            }
            bool wasActive = host.activeSelf;
            host.SetActive(false);                       // assign settings before the document enables
            var doc = host.AddComponent<UIDocument>();
            doc.panelSettings = settings;
            doc.visualTreeAsset = Resources.Load<VisualTreeAsset>(uxmlResource);
            var font = Resources.Load<Font>("gun4f");
            if (font != null) doc.rootVisualElement.style.unityFontDefinition = new StyleFontDefinition(FontDefinition.FromFont(font));
            host.SetActive(wasActive);
            foreach (var uss in ussResources)
            {
                var sheet = Resources.Load<StyleSheet>(uss);
                if (sheet != null) doc.rootVisualElement.styleSheets.Add(sheet);
            }
            return doc;
        }

        public static bool IsPointerOver(UIDocument doc)
        {
            if (doc == null || doc.rootVisualElement == null || doc.rootVisualElement.panel == null) return false;
            var panel = doc.rootVisualElement.panel;
            Vector2 screen = new Vector2(Input.mousePosition.x, Screen.height - Input.mousePosition.y);
            var pos = RuntimePanelUtils.ScreenToPanel(panel, screen);
            var picked = panel.Pick(pos);
            return picked != null && picked != doc.rootVisualElement;
        }
    }
}
