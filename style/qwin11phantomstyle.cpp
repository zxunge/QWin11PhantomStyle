#include "qwin11phantomstyle.h"
#include "qwin11phantomcolor.h"
#include "qwin11phantomtweak.h"

#include <QtCore/qmath.h>
#include <QtCore/qpoint.h>
#include <QtCore/qshareddata.h>
#include <QtCore/qsharedpointer.h>
#include <QtCore/qstring.h>
#include <QtCore/qstringview.h>
#include <QtGui/qfont.h>
#include <QtGui/qpainter.h>
#include <QtGui/qpainterpath.h>
#include <QtGui/qpolygon.h>
#include <QtGui/qwindow.h>
#include <QtWidgets/qabstractscrollarea.h>
#include <QtWidgets/qapplication.h>
#include <QtWidgets/qdrawutil.h>
#include <QtWidgets/qstyleoption.h>
#if QT_CONFIG(combobox)
#  include <QtWidgets/qcombobox.h>
#endif
#if QT_CONFIG(pushbutton)
#  include <QtWidgets/qpushbutton.h>
#endif
#if QT_CONFIG(abstractbutton)
#  include <QtWidgets/qabstractbutton.h>
#endif
#if QT_CONFIG(mainwindow)
#  include <QtWidgets/qmainwindow.h>
#endif
#if QT_CONFIG(groupbox)
#  include <QtWidgets/qgroupbox.h>
#endif
#if QT_CONFIG(scrollbar)
#  include <QtWidgets/qscrollbar.h>
#endif
#if QT_CONFIG(spinbox)
#  include <QtWidgets/qspinbox.h>
#endif
#if QT_CONFIG(abstractslider)
#  include <QtWidgets/qabstractslider.h>
#endif
#if QT_CONFIG(slider)
#  include <QtWidgets/qslider.h>
#endif
#if QT_CONFIG(splitter)
#  include <QtWidgets/qsplitter.h>
#endif
#if QT_CONFIG(progressbar)
#  include <QtWidgets/qprogressbar.h>
#endif
#if QT_CONFIG(wizard)
#  include <QtWidgets/qwizard.h>
#endif
#if QT_CONFIG(scrollbar)
#  include <QtWidgets/qscrollbar.h>
#endif
#if QT_CONFIG(menu)
#  include <QtWidgets/qmenu.h>
#endif
#if QT_CONFIG(toolbar)
#  include <QtWidgets/qtoolbar.h>
#endif
#if QT_CONFIG(toolbutton)
#  include <QtWidgets/qtoolbutton.h>
#endif
#if QT_CONFIG(dialogbuttonbox)
#  include <QtWidgets/qdialogbuttonbox.h>
#endif
#if QT_CONFIG(itemviews)
#  include <QtWidgets/qabstractitemview.h>
#  include <QtWidgets/qheaderview.h>
#  include <QtWidgets/qlistview.h>
#  include <QtWidgets/qtableview.h>
#  include <QtWidgets/qtreeview.h>
#endif

#ifdef BUILD_WITH_EASY_PROFILER
#  include <QtCore/qmetaobject.h>
#  include <easy/arbitrary_value.h>
#  include <easy/profiler.h>
#endif

#include <cmath>

QT_BEGIN_NAMESPACE
Q_GUI_EXPORT int qt_defaultDpiX();
QT_END_NAMESPACE

namespace QWin11Phantom {
namespace Tweak {
const char *const menubar_no_ruler = "_phantom_menubar_no_ruler";
}
namespace {
Q_NEVER_INLINE bool hasTweakTrue(const QObject *object, const char *tweakName)
{
    if (!object)
        return false;
    QVariant value = object->property(tweakName);
    return value.toBool();
}
} // namespace
} // namespace QWin11Phantom

namespace QWin11Phantom {
namespace {
enum {
    MenuMinimumWidth = 10, // Smallest width that menu items can have
    SplitterMaxLength = 25, // Length of splitter handle (not thickness)
    SpinBox_ButtonWidth = 14,

    // These two are currently not based on font, but could be
    LineEdit_ContentsHPad = 2,
    ComboBox_NonEditable_ContentsHPad = 4,
};

static const qreal Default_Rounding = 6.0;
static const qreal Default_Thickness = 0.3;
static const qreal Edge_Default_Thickness = 1.0;
static const qreal TabBarTab_Rounding = 4.0;
static const qreal SliderHandle_Rounding = 2.0;

static const qreal SliderDefaultWidth = 2.0;

static const qreal Frame_Selected_Edge_Width = 2.0;

static const qreal CheckMark_WidthOfHeightScale = 0.8;
static const qreal PushButton_HorizontalPaddingFontHeightRatio = 1.0 / 2.0;
static const qreal TabBar_HPaddingFontRatio = 1.25;
static const qreal TabBar_VPaddingFontRatio = 1.0 / 1.25;
static const qreal GroupBox_LabelBottomMarginFontRatio = 1.0 / 4.0;
static const qreal ComboBox_ArrowMarginRatio = 1.0 / 3.25;

static const qreal MenuBar_HorizontalPaddingFontRatio = 1.0 / 2.0;
static const qreal MenuBar_VerticalPaddingFontRatio = 1.0 / 6.0;

static const qreal MenuItem_LeftMarginFontRatio = 1.0 / 4.0;
static const qreal MenuItem_RightMarginForTextFontRatio = 1.0 / 1.5;
static const qreal MenuItem_RightMarginForArrowFontRatio = 1.0 / 4.0;
static const qreal MenuItem_VerticalMarginsFontRatio = 1.0 / 8.0;
// Number that's multiplied with a font's height to get the space between a
// menu item's checkbox (or other sign) and its text (or icon).
static const qreal MenuItem_CheckRightSpaceFontRatio = 1.0 / 4.0;
static const qreal MenuItem_TextMnemonicSpaceFontRatio = 1.5;
static const qreal MenuItem_SubMenuArrowSpaceFontRatio = 1.0 / 1.5;
static const qreal MenuItem_SubMenuArrowWidthFontRatio = 1.0 / 2.75;
static const qreal MenuItem_SeparatorHeightFontRatio = 1.0 / 1.5;
static const qreal MenuItem_CheckMarkVerticalInsetFontRatio = 1.0 / 10.0;
static const qreal MenuItem_IconRightSpaceFontRatio = 1.0 / 3.0;

static const bool BranchesOnEdge = false;
static const bool MenuExtraBottomMargin = true;
static const bool MenuBarLeftMargin = false;
// Note that this only applies to the disclosure etc. decorators in tree views.
static const bool ShowItemViewDecorationSelected = false;
static const bool UseQMenuForComboBoxPopup = true;
static const bool ItemView_UseFontHeightForDecorationSize = true;

// Whether or not the non-raised tabs in a tab bar have shininess/highlights to
// them. Setting this to false adds an extra visual hint for distinguishing
// between the current and non-current tabs, but makes the non-current tabs
// appear less clickable. Other ways to increase the visual differences could
// be to increase the color contrast for the background fill color, or increase
// the vertical offset. However, increasing the vertical offset comes with some
// layout challenges, and increasing the color contrast further may visually
// imply an incorrect layout structure. Not sure what's best.
//
// This doesn't disable creating the color/brush resource, even though it's
// currently a compile-time-only option, because it may be changed to be part
// of some dynamic config system for Phantom in the future, or have a
// per-widget style hint associated with it.
static const bool TabBar_InactiveTabsHaveSpecular = false;

struct Grad
{
    Grad(const QColor &from, const QColor &to)
    {
        rgbA = Rgb::ofQColor(from);
        rgbB = Rgb::ofQColor(to);
        lA = rgbA.toHsl().l;
        lB = rgbB.toHsl().l;
    }
    QColor sample(qreal alpha) const
    {
        Hsl hsl = Rgb::lerp(rgbA, rgbB, alpha).toHsl();
        hsl.l = QWin11Phantom::lerp(lA, lB, alpha);
        return hsl.toQColor();
    }
    Rgb rgbA, rgbB;
    qreal lA, lB;
};

namespace DeriveColors {
Q_NEVER_INLINE QColor adjustLightness(const QColor &qcolor, qreal ld)
{
    Hsl hsl = Hsl::ofQColor(qcolor);
    const qreal gamma = 3.0;
    hsl.l = std::pow(QWin11Phantom::saturate(std::pow(hsl.l, 1.0 / gamma) + ld * 0.8), gamma);
    return hsl.toQColor();
}
QColor buttonColor(const QPalette &pal)
{
    // temp hack
    if (pal.color(QPalette::Button) == pal.color(QPalette::Window))
        return adjustLightness(pal.color(QPalette::Button), 0.01);
    return pal.color(QPalette::Button);
}
QColor highlightedOutlineOf(const QPalette &pal)
{
    return adjustLightness(pal.color(QPalette::Highlight), -0.05);
}
QColor dividerColor(const QColor &underlying)
{
    return adjustLightness(underlying, -0.05);
}
QColor outlineOf(const QPalette &pal)
{
    return adjustLightness(pal.color(QPalette::Window), -0.1);
}
QColor lightShadeOf(const QColor &underlying)
{
    return adjustLightness(underlying, 0.07);
}
QColor darkShadeOf(const QColor &underlying)
{
    return adjustLightness(underlying, -0.07);
}
QColor overhangShadowOf(const QColor &underlying)
{
    return adjustLightness(underlying, -0.05);
}

QColor specularOf(const QColor &underlying)
{
    return adjustLightness(underlying, 0.03);
}
QColor pressedOf(const QColor &color)
{
    return adjustLightness(color, 0.02);
}
QColor indicatorColorOf(const QPalette &palette, QPalette::ColorGroup group = QPalette::Current)
{
    return Grad(palette.color(group, QPalette::WindowText), palette.color(group, QPalette::Button))
            .sample(0.45);
}
QColor inactiveTabFillColorOf(const QColor &underlying)
{
    // used to be -0.01
    return adjustLightness(underlying, -0.025);
}
QColor progressBarOutlineColorOf(const QPalette &pal)
{
    // Pretty wasteful
    Hsl hsl0 = Hsl::ofQColor(pal.color(QPalette::Window));
    Hsl hsl1 = Hsl::ofQColor(pal.color(QPalette::Highlight));
    hsl1.l = saturate(qMin(hsl0.l - 0.1, hsl1.l - 0.2));
    return hsl1.toQColor();
}
QColor itemViewMultiSelectionCurrentBorderOf(const QPalette &pal)
{
    return adjustLightness(pal.color(QPalette::Highlight), -0.15);
}
bool hack_isLightPalette(const QPalette &pal)
{
    Hsl hsl0 = Hsl::ofQColor(pal.color(QPalette::WindowText));
    Hsl hsl1 = Hsl::ofQColor(pal.color(QPalette::Window));
    return hsl0.l < hsl1.l;
}
QColor itemViewHeaderOnLineColorOf(const QPalette &pal)
{
    return hack_isLightPalette(pal)
            ? highlightedOutlineOf(pal)
            : Grad(pal.color(QPalette::WindowText), pal.color(QPalette::Window)).sample(0.5);
}
} // namespace DeriveColors

namespace SwatchColors {
enum SwatchColor {
    S_none = 0,
    S_window,
    S_button,
    S_base,
    S_text,
    S_windowText,
    S_highlight,
    S_highlightedText,
    S_window_outline,
    S_window_specular,
    S_window_divider,
    S_window_lighter,
    S_window_darker,
    S_button_specular,
    S_button_pressed,
    S_button_on,
    S_button_hover,
    S_button_pressed_specular,
    S_base_shadow,
    S_base_divider,
    S_windowText_disabled,
    S_highlight_outline,
    S_highlight_specular,
    S_progressBar_outline,
    S_inactiveTabYesFrame,
    S_inactiveTabNoFrame,
    S_inactiveTabYesFrame_specular,
    S_inactiveTabNoFrame_specular,
    S_indicator_current,
    S_indicator_disabled,
    S_itemView_multiSelection_currentBorder,
    S_itemView_headerOnLine,

    S_last, /* Needs to be last */

    // Aliases
    S_progressBar = S_highlight,
    S_progressBar_specular = S_highlight_specular,
    S_tabFrame = S_window,
    S_tabFrame_specular = S_window_specular,
};
}

using Swatchy = SwatchColors::SwatchColor;

struct PhSwatch : public QSharedData
{
    // The pens store the brushes within them, so storing the brushes here as
    // well is redundant. However, QPen::brush() does not return its brush by
    // reference, so we'd end up doing a bunch of inc/dec work every time we use
    // one. Also, it saves us the indirection of chasing two pointers (Swatch ->
    // QPen -> QBrush) every time we want to get a QColor.
    QColor colors[SwatchColors::S_last + 1];
    // Note: the casts to int in the assert macros are to suppress a false
    // positive warning for tautological comparison in the clang linter.
    const QColor &color(Swatchy swatchValue) const
    {
        Q_ASSERT((int)swatchValue >= 0 && (int)swatchValue < SwatchColors::S_last + 1);
        return colors[swatchValue];
    }
    const QPen pen(Swatchy swatchValue) const
    {
        Q_ASSERT((int)swatchValue >= 0 && (int)swatchValue < SwatchColors::S_last + 1);
        if (swatchValue == SwatchColors::S_none) {
            return Qt::NoPen;
        }

        return QPen(colors[swatchValue], 0.5);
    }

    void loadFromQPalette(const QPalette &pal);
};

using PhSwatchPtr = QExplicitlySharedDataPointer<PhSwatch>;
using PhCacheEntry = QPair<uint, PhSwatchPtr>;
enum : int {
    Num_ColorCacheEntries = 10,
};
using PhSwatchCache = QVarLengthArray<PhCacheEntry, Num_ColorCacheEntries>;
Q_NEVER_INLINE void PhSwatch::loadFromQPalette(const QPalette &pal)
{
    using namespace SwatchColors;
    namespace Dc = DeriveColors;
    const bool isEnabled = pal.currentColorGroup() != QPalette::Disabled;

    colors[S_none] = Qt::transparent;

    colors[S_window] = pal.color(QPalette::Window);
    colors[S_button] = pal.color(QPalette::Button);
    if (colors[S_button] == colors[S_window])
        colors[S_button] = Dc::adjustLightness(colors[S_button], 0.01);
    colors[S_base] = pal.color(QPalette::Base);
    colors[S_text] = pal.color(QPalette::Text);
    colors[S_windowText] = pal.color(QPalette::WindowText);
    colors[S_highlight] = pal.color(QPalette::Highlight);
    colors[S_highlightedText] = pal.color(QPalette::HighlightedText);

    // There's a chance that some widgets won't redraw when changing to and from
    // the disabled state, causing the outline color in the frame buffer to be
    // out of sync with what it should be. If we notice this problem, we can get
    // rid of conditional color branching and try to do it some other way.
    colors[S_window_outline] = Dc::adjustLightness(colors[S_window], isEnabled ? -0.1 : -0.07);
    colors[S_window_specular] = isEnabled ? Dc::specularOf(colors[S_window]) : colors[S_window];
    colors[S_window_divider] = Dc::dividerColor(colors[S_window]);
    colors[S_window_lighter] = Dc::lightShadeOf(colors[S_window]);
    colors[S_window_darker] = Dc::darkShadeOf(colors[S_window]);
    colors[S_button_specular] = isEnabled ? Dc::specularOf(colors[S_button]) : colors[S_button];
    colors[S_button_pressed] = Dc::pressedOf(colors[S_button]);
    colors[S_button_on] = Dc::adjustLightness(colors[S_window], isEnabled ? -0.05 : -0.07);
    colors[S_button_hover] = Dc::adjustLightness(colors[S_button], -0.01);
    ;

    colors[S_button_pressed_specular] =
            isEnabled ? Dc::specularOf(colors[S_button_pressed]) : colors[S_button_pressed];
    colors[S_base_shadow] = Dc::overhangShadowOf(colors[S_base]);
    colors[S_base_divider] = Dc::dividerColor(colors[S_base]);
    colors[S_windowText_disabled] = pal.color(QPalette::Disabled, QPalette::WindowText);
    colors[S_highlight_outline] = Dc::adjustLightness(colors[S_highlight], -0.05);
    colors[S_highlight_specular] =
            isEnabled ? Dc::specularOf(colors[S_highlight]) : colors[S_highlight];
    colors[S_progressBar_outline] = Dc::progressBarOutlineColorOf(pal);
    colors[S_inactiveTabYesFrame] = Dc::inactiveTabFillColorOf(colors[S_tabFrame]);
    colors[S_inactiveTabNoFrame] = Dc::inactiveTabFillColorOf(colors[S_window]);
    colors[S_inactiveTabYesFrame_specular] = isEnabled
            ? Dc::specularOf(colors[S_inactiveTabYesFrame])
            : colors[S_inactiveTabYesFrame];
    colors[S_inactiveTabNoFrame_specular] = Dc::specularOf(colors[S_inactiveTabNoFrame]);
    colors[S_indicator_current] = Dc::indicatorColorOf(pal, QPalette::Current);
    colors[S_indicator_disabled] = Dc::indicatorColorOf(pal, QPalette::Disabled);
    colors[S_itemView_multiSelection_currentBorder] =
            Dc::itemViewMultiSelectionCurrentBorderOf(pal);
    colors[S_itemView_headerOnLine] = Dc::itemViewHeaderOnLineColorOf(pal);
}

// This is the "hash" (not really a hash) function we'll use on the happy fast
// path when looking up a PhSwatch for a given QPalette. It's fragile, because
// it uses QPalette::cacheKey(), so it may not match even when the contents
// (currentColorGroup + the RGB colors) of the QPalette are actually a match.
// But it's cheaper to calculate, so we'll store a single one of these "hashes"
// for the head (most recently used) cached PhSwatch, and check to see if it
// matches. This is the most common case, so we can usually save some work by
// doing this. (The second most common case is probably having a different
// ColorGroup but the rest of the contents are the same, but we don't have a
// special path for that.)
quint64 fastfragile_hash_qpalette(const QPalette &p)
{
    union {
        qint64 i;
        quint64 u;
    } x;
    x.i = p.cacheKey();
    // QPalette::ColorGroup has range 0..5 (inclusive), so it only uses 3 bits.
    // The high 32 bits in QPalette::cacheKey() are a global incrementing serial
    // number for the QPalette creation. We don't store (2^29-1) things in our
    // cache, and I doubt that many will ever be created in a real application
    // while also retaining some of them across such a broad time range, so it's
    // really unlikely that repurposing these top 3 bits to also include the
    // QPalette::currentColorGroup() (which the cacheKey doesn't include for some
    // reason...) will generate a collision.
    //
    // This may not be true in the future if the way the QPalette::cacheKey() is
    // generated changes. If that happens, change to use the definition of
    // `fastfragile_hash_qpalette` below, which is less likely to collide with an
    // arbitrarily numbered key but also does more work.
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    x.u = x.u ^ ((quint64)p.currentColorGroup() << (64 - 3));
    return x.u;
#else
    // Use this definition here if the contents/layout of QPalette::cacheKey()
    // (as in, the C++ code in qpalette.cpp) are changed. We'll also put a Qt6
    // guard for it, so that it will default to a more safe definition on the
    // next guaranteed big breaking change for Qt. A warning will hopefully get
    // someone to double-check it at some point in the future.
#  ifdef _MSC_VER
#    pragma message("Verify contents and layout of QPalette::cacheKey() have not changed.")
#  else
#    warning "Verify contents and layout of QPalette::cacheKey() have not changed."
#  endif
    QtPrivate::QHashCombine c;
    uint h = qHash(p.currentColorGroup());
    h = c(h, (uint)(x.u & 0xFFFFFFFFu));
    h = c(h, (uint)((x.u >> 32) & 0xFFFFFFFFu));
    return h;
#endif
}

// This hash function is for when we want an actual accurate hash of a
// QPalette. QPalette's cacheKey() isn't very reliable -- it seems to change to
// a new random number whenever it's modified, with the exception of the
// currentColorGroup being changed. This kind of sucks for us, because it means
// two QPalette's can have the same contents but hash to different values. And
// this actually happens a lot! We'll do the hashing ourselves. Also, we're not
// interested in all of the colors, only some of them, and we ignore
// pens/brushes.
uint accurate_hash_qpalette(const QPalette &p)
{
    // Probably shouldn't use this, could replace with our own guy. It's not a
    // great hasher anyway.
    QtPrivate::QHashCombine c;
    uint h = qHash(p.currentColorGroup());
    QPalette::ColorRole const roles[] = { QPalette::Window,         QPalette::Button,
                                          QPalette::Base,           QPalette::Text,
                                          QPalette::WindowText,     QPalette::Highlight,
                                          QPalette::HighlightedText };
    for (int i = 0; i < (int)(sizeof(roles) / sizeof(roles[0])); ++i) {
        h = c(h, p.color(roles[i]).rgb());
    }
    return h;
}

Q_NEVER_INLINE PhSwatchPtr
deep_getCachedSwatchOfQPalette(PhSwatchCache *cache,
                               int cacheCount, // Just saving a call to cache->count()
                               const QPalette &qpalette)
{
    // Calculate our hash key from the QPalette's current ColorGroup and the
    // actual RGBA values that we use. We have to mix the ColorGroup in
    // ourselves, because QPalette does not account for it in the cache key.
    uint key = accurate_hash_qpalette(qpalette);
    int n = cacheCount;
    int idx = -1;
    for (int i = 0; i < n; ++i) {
        const auto &x = cache->at(i);
        if (x.first == key) {
            idx = i;
            break;
        }
    }
    if (idx == -1) {
        PhSwatchPtr ptr;
        if (n < Num_ColorCacheEntries) {
            ptr = new PhSwatch;
        } else {
            // Remove the oldest guy from the cache. Remember that because we may
            // re-enter QStyle functions multiple times when drawing or calculating
            // something, we may have to load several swaitches derived from
            // different QPalettes on different stack frames at the same time. But as
            // an extra cost-savings measure, we'll check and see if something else
            // has a reference to the removed guy. If there aren't any references to
            // it, then we'll re-use it directly instead of allocating a new one. (We
            // will only ever run into the case where we can't re-use it directly if
            // some other stack frame has a reference to it.) This is nice because
            // then the QPens and QBrushes don't all also have to reallocate their d
            // ptr stuff.
            ptr = cache->last().second;
            cache->removeLast();
            ptr.detach();
        }
        ptr->loadFromQPalette(qpalette);
        cache->insert(cache->cbegin(), PhCacheEntry(key, ptr));
        return ptr;
    } else {
        if (idx == 0) {
            return cache->at(idx).second;
        }
        PhCacheEntry e = cache->at(idx);
        // Using std::move from algorithm could be more efficient here, but I don't
        // want to depend on algorithm or write this myself. Small N with a movable
        // type means it doesn't really matter in this case.
        cache->remove(idx);
        cache->insert(cache->cbegin(), e);
        return e.second;
    }
}

Q_NEVER_INLINE PhSwatchPtr
getCachedSwatchOfQPalette(PhSwatchCache *cache,
                          quint64 *headSwatchFastKey, // Optimistic fast-path quick hash key
                          const QPalette &qpalette)
{
    quint64 ck = fastfragile_hash_qpalette(qpalette);
    int cacheCount = cache->count();
    // This hint is counter-productive if we're being called in a way that
    // interleaves different QPalettes. But misses to this optimistic path were
    // rare in my tests. (Probably not going to amount to any significant
    // difference, anyway.)
    if (Q_LIKELY(cacheCount > 0 && *headSwatchFastKey == ck)) {
        return cache->at(0).second;
    }
    *headSwatchFastKey = ck;
    return deep_getCachedSwatchOfQPalette(cache, cacheCount, qpalette);
}

} // namespace
} // namespace QWin11Phantom

class QWin11PhantomStylePrivate
{
public:
    QWin11PhantomStylePrivate();

    // A fast'n'easy hash of QPalette::cacheKey()+QPalette::currentColorGroup()
    // of only the head element of swatchCache list. The most common thing that
    // happens when deriving a PhSwatch from a QPalette is that we just end up
    // re-using the last one that we used. For that case, we can potentially save
    // calling `accurate_hash_qpalette()` and instead use the value returned by
    // QPalette::cacheKey() (and QPalette::currentColorGroup()) and compare it to
    // the last one that we used. If it matches, then we know we can just use the
    // head of the cache list without having to do any further checks, which
    // saves a few hundred (!) nanoseconds.
    //
    // However, the `QPalette::cacheKey()` value is fragile and may change even
    // if none of the colors in the QPalette have changed. In other words, all of
    // the colors in a QPalette may match another QPalette (or a derived
    // PhSwatch) even if the `QPalette::cacheKey()` value is different.
    //
    // So if `QPalette::cacheKey()+currentColorGroup()` doesn't match, then we'll
    // use our more accurate `accurate_hash_qpalette()` to get a more accurate
    // comparison key, and then search through the cache list to find a matching
    // cached PhSwatch. (The more accurate cache key is what we store alongside
    // each PhSwatch element, as the `.first` in each QPair. The
    // QPalette::cacheKey() that we associate with the PhSwatch in the head
    // position, `headSwatchFastKey`, is only stored for our single head element,
    // as a special fast case.) If we find it, we'll move it to the head of the
    // cache list. If not, we'll make a new one, and put it at the head. Either
    // way, the `headSwatchFastKey` will be updated to the
    // `fastfragile_qpalette_hash()` of the QPalette that we needed to derive a
    // PhSwatch from, so that if we get called with the same QPalette again next
    // time (which is probably going to be the case), it'll match and we can take
    // the fast path.
    quint64 headSwatchFastKey;

    QWin11Phantom::PhSwatchCache swatchCache;
    QPen checkBox_pen_scratch;
};

namespace QWin11Phantom {
namespace {

// Minimal QPainter save/restore just for pen, brush, and AA render hint. If
// you touch more than that, this won't help you. But if you're only touching
// those things, this will save you some typing from manually storing/saving
// those properties each time.
struct PSave final
{
    Q_DISABLE_COPY(PSave)

    explicit PSave(QPainter *painter_)
    {
        Q_ASSERT(painter_);
        painter = painter_;
        pen = painter_->pen();
        brush = painter_->brush();
        hintAA = painter_->testRenderHint(QPainter::Antialiasing);
    }
    Q_NEVER_INLINE void restore()
    {
        QPainter *p = painter;
        if (!p)
            return;
        bool hintAA_ = hintAA;
        // QPainter will check both pen and brush for equality when setting, so we
        // should set it unconditionally here.
        p->setPen(pen);
        p->setBrush(brush);
        // But it won't check the render hint to guard against doing extra work.
        // We'll do that ourselves. (Though at least for the raster engine, this
        // doesn't cause very much work to occur. But it still chases a few
        // pointers.)
        if (p->testRenderHint(QPainter::Antialiasing) != hintAA_) {
            p->setRenderHint(QPainter::Antialiasing, hintAA_);
        }
        painter = nullptr;
        pen = QPen();
        brush = QBrush();
        hintAA = false;
    }
    ~PSave() { restore(); }

private:
    QPainter *painter;
    QPen pen;
    QBrush brush;
    bool hintAA;
};

static const qreal Pi = qreal(M_PI);

qreal dpiScaled(qreal value)
{
#ifdef Q_OS_MAC
    // On mac the DPI is always 72 so we should not scale it
    return value;
#else
    static const qreal scale = qreal(qt_defaultDpiX()) / 96.0;
    return value * scale;
#endif
}

#if QT_CONFIG(menu)
struct MenuItemMetrics
{
    int fontHeight;
    int frameThickness;
    int leftMargin;
    int rightMarginForText;
    int rightMarginForArrow;
    int topMargin;
    int bottomMargin;
    int checkWidth;
    int checkRightSpace;
    int iconRightSpace;
    int mnemonicSpace;
    int arrowSpace;
    int arrowWidth;
    int separatorHeight;
    int totalHeight;

    static MenuItemMetrics ofFontHeight(int fontHeight);

private:
    MenuItemMetrics() { }
};

MenuItemMetrics MenuItemMetrics::ofFontHeight(int fontHeight)
{
    MenuItemMetrics m;
    m.fontHeight = fontHeight;
    m.frameThickness = (int)dpiScaled(1.0);
    m.leftMargin = (int)((qreal)fontHeight * MenuItem_LeftMarginFontRatio);
    m.rightMarginForText = (int)((qreal)fontHeight * MenuItem_RightMarginForTextFontRatio);
    m.rightMarginForArrow = (int)((qreal)fontHeight * MenuItem_RightMarginForArrowFontRatio);
    m.topMargin = (int)((qreal)fontHeight * MenuItem_VerticalMarginsFontRatio);
    m.bottomMargin = (int)((qreal)fontHeight * MenuItem_VerticalMarginsFontRatio);
    int checkVMargin = (int)((qreal)fontHeight * MenuItem_CheckMarkVerticalInsetFontRatio);
    int checkHeight = fontHeight - checkVMargin * 2;
    if (checkHeight < 0)
        checkHeight = 0;
    m.checkWidth = (int)((qreal)checkHeight * CheckMark_WidthOfHeightScale);
    m.checkRightSpace = (int)((qreal)fontHeight * MenuItem_CheckRightSpaceFontRatio);
    m.iconRightSpace = (int)((qreal)fontHeight * MenuItem_IconRightSpaceFontRatio);
    m.mnemonicSpace = (int)((qreal)fontHeight * MenuItem_TextMnemonicSpaceFontRatio);
    m.arrowSpace = (int)((qreal)fontHeight * MenuItem_SubMenuArrowSpaceFontRatio);
    m.arrowWidth = (int)((qreal)fontHeight * MenuItem_SubMenuArrowWidthFontRatio);
    m.separatorHeight = (int)((qreal)fontHeight * MenuItem_SeparatorHeightFontRatio);
    // Odd numbers only
    m.separatorHeight = (m.separatorHeight / 2) * 2 + 1;
    m.totalHeight = fontHeight + m.frameThickness * 2 + m.topMargin + m.bottomMargin;
    return m;
}

QRect menuItemContentRect(const MenuItemMetrics &metrics, QRect itemRect, bool hasArrow)
{
    QRect r = itemRect;
    int ft = metrics.frameThickness;
    int rm = hasArrow ? metrics.rightMarginForArrow : metrics.rightMarginForText;
    r.adjust(ft + metrics.leftMargin, ft + metrics.topMargin, -(ft + rm),
             -(ft + metrics.bottomMargin));
    return r.isValid() ? r : QRect();
}
QRect menuItemCheckRect(const MenuItemMetrics &metrics, Qt::LayoutDirection direction,
                        QRect itemRect, bool hasArrow)
{
    QRect r = menuItemContentRect(metrics, itemRect, hasArrow);
    int checkVMargin = (int)((qreal)metrics.fontHeight * MenuItem_CheckMarkVerticalInsetFontRatio);
    if (checkVMargin < 0)
        checkVMargin = 0;
    r.setSize(QSize(metrics.checkWidth, metrics.fontHeight));
    r.adjust(0, checkVMargin, 0, -checkVMargin);
    return QStyle::visualRect(direction, itemRect, r) & itemRect;
}
QRect menuItemIconRect(const MenuItemMetrics &metrics, Qt::LayoutDirection direction,
                       QRect itemRect, bool hasArrow)
{
    QRect r = menuItemContentRect(metrics, itemRect, hasArrow);
    r.setX(r.x() + metrics.checkWidth + metrics.checkRightSpace);
    r.setSize(QSize(metrics.fontHeight, metrics.fontHeight));
    return QStyle::visualRect(direction, itemRect, r) & itemRect;
}
QRect menuItemTextRect(const MenuItemMetrics &metrics, Qt::LayoutDirection direction,
                       QRect itemRect, bool hasArrow, bool hasIcon, int tabWidth)
{
    QRect r = menuItemContentRect(metrics, itemRect, hasArrow);
    r.setX(r.x() + metrics.checkWidth + metrics.checkRightSpace);
    if (hasIcon) {
        r.setX(r.x() + metrics.fontHeight + metrics.iconRightSpace);
    }
    r.setWidth(r.width() - tabWidth);
    r.setHeight(metrics.fontHeight);
    r &= itemRect;
    return QStyle::visualRect(direction, itemRect, r);
}
QRect menuItemMnemonicRect(const MenuItemMetrics &metrics, Qt::LayoutDirection direction,
                           QRect itemRect, bool hasArrow, int tabWidth)
{
    QRect r = menuItemContentRect(metrics, itemRect, hasArrow);
    int x = r.x() + r.width() - tabWidth;
    if (hasArrow)
        x -= metrics.arrowSpace + metrics.arrowWidth;
    r.setX(x);
    r.setHeight(metrics.fontHeight);
    r &= itemRect;
    return QStyle::visualRect(direction, itemRect, r);
}

#endif

QRect progressBarRect(const QStyleOptionProgressBar *bar)
{
    bool isHorizontal = !bar->bottomToTop;
    QSize size = bar->rect.size();

    /* Make the progessbar width to be no bigger rthen 6 */
    if (isHorizontal) {
        size.setHeight(qMin(size.height(), 8));
    } else {
        size.setWidth(qMin(size.width(), 8));
    }

    return QStyle::alignedRect(Qt::LayoutDirection::LayoutDirectionAuto, Qt::AlignCenter, size,
                               bar->rect);
}

void progressBarFillRects(const QStyleOptionProgressBar *bar,
                          // The rect that represents the filled/completed region
                          QRect &outFilled,
                          // The rect that represents the incomplete region
                          QRect &outNonFilled,
                          // Whether or not the progress bar is indeterminate
                          bool &outIsIndeterminate)
{
    bool isHorizontal = !bar->bottomToTop;
    QRect ra = progressBarRect(bar);
    QRect rb = ra;
    bool isInverted = bar->invertedAppearance;
    bool isIndeterminate = bar->minimum == 0 && bar->maximum == 0;
    bool isForward = !isHorizontal || bar->direction != Qt::RightToLeft;
    if (isInverted)
        isForward = !isForward;
    int maxLen = isHorizontal ? ra.width() : ra.height();
    const auto availSteps = qMax(Q_INT64_C(1), qint64(bar->maximum) - bar->minimum);
    const auto progress = qMax(bar->progress, bar->minimum); // workaround for bug in QProgressBar
    const auto progressSteps = qint64(progress) - bar->minimum;
    const auto progressBarWidth = progressSteps * maxLen / availSteps;
    int barLen = isIndeterminate ? maxLen : (int)progressBarWidth;
    if (isHorizontal) {
        if (isForward) {
            ra.setWidth(barLen);
            rb.setX(barLen);
        } else {
            ra.setX(ra.x() + ra.width() - barLen);
            rb.setWidth(rb.width() - barLen);
        }
    } else {
        if (isForward) {
            ra.setY(ra.y() + ra.height() - barLen);
            rb.setHeight(rb.height() - barLen);
        } else {
            ra.setHeight(barLen);
            rb.setY(barLen);
        }
    }
    outFilled = ra;
    outNonFilled = rb;
    outIsIndeterminate = isIndeterminate;
}

#if QT_CONFIG(dial)
int calcBigLineSize(int radius)
{
    int bigLineSize = radius / 6;
    if (bigLineSize < 4)
        bigLineSize = 4;
    if (bigLineSize > radius / 2)
        bigLineSize = radius / 2;
    return bigLineSize;
}
Q_NEVER_INLINE QPointF calcRadialPos(const QStyleOptionSlider *dial, qreal offset)
{
    const int width = dial->rect.width();
    const int height = dial->rect.height();
    const int r = qMin(width, height) / 2;
    const int currentSliderPosition =
            dial->upsideDown ? dial->sliderPosition : (dial->maximum - dial->sliderPosition);
    qreal a = 0;
    if (dial->maximum == dial->minimum)
        a = Pi / 2;
    else if (dial->dialWrapping)
        a = Pi * 3 / 2
                - (currentSliderPosition - dial->minimum) * 2 * Pi
                        / (dial->maximum - dial->minimum);
    else
        a = (Pi * 8
             - (currentSliderPosition - dial->minimum) * 10 * Pi / (dial->maximum - dial->minimum))
                / 6;
    qreal xc = width / 2.0;
    qreal yc = height / 2.0;
    qreal len = r - calcBigLineSize(r) - 3;
    qreal back = offset * len;
    QPointF pos(QPointF(xc + back * qCos(a), yc - back * qSin(a)));
    return pos;
}
Q_NEVER_INLINE QPolygonF calcLines(const QStyleOptionSlider *dial)
{
    QPolygonF poly;
    qreal width = (qreal)dial->rect.width();
    qreal height = (qreal)dial->rect.height();
    qreal r = qMin(width, height) / 2.0;
    int bigLineSize = calcBigLineSize(int(r));

    qreal xc = width / 2.0 + 0.5;
    qreal yc = height / 2.0 + 0.5;
    const int ns = dial->tickInterval;
    if (!ns) // Invalid values may be set by Qt Designer.
        return poly;
    int notches = (dial->maximum + ns - 1 - dial->minimum) / ns;
    if (notches <= 0)
        return poly;
    if (dial->maximum < dial->minimum || dial->maximum - dial->minimum > 1000) {
        int maximum = dial->minimum + 1000;
        notches = (maximum + ns - 1 - dial->minimum) / ns;
    }
    poly.resize(2 + 2 * notches);
    int smallLineSize = bigLineSize / 2;
    for (int i = 0; i <= notches; ++i) {
        qreal angle = dial->dialWrapping ? Pi * 3 / 2 - i * 2 * Pi / notches
                                         : (Pi * 8 - i * 10 * Pi / notches) / 6;
        qreal s = qSin(angle);
        qreal c = qCos(angle);
        if (i == 0 || (((ns * i) % (dial->pageStep ? dial->pageStep : 1)) == 0)) {
            poly[2 * i] = QPointF(xc + (r - bigLineSize) * c, yc - (r - bigLineSize) * s);
            poly[2 * i + 1] = QPointF(xc + r * c, yc - r * s);
        } else {
            poly[2 * i] =
                    QPointF(xc + (r - 1 - smallLineSize) * c, yc - (r - 1 - smallLineSize) * s);
            poly[2 * i + 1] = QPointF(xc + (r - 1) * c, yc - (r - 1) * s);
        }
    }
    return poly;
}
// This will draw a nice and shiny QDial for us. We don't want
// all the shinyness in QWindowsStyle, hence we place it here
Q_NEVER_INLINE void drawDial(const QStyleOptionSlider *option, QPainter *painter)
{
    namespace Dc = QWin11Phantom::DeriveColors;
    const QPalette &pal = option->palette;
    QColor buttonColor = Dc::buttonColor(option->palette);
    const int width = option->rect.width();
    const int height = option->rect.height();
    const bool enabled = option->state & QStyle::State_Enabled;
    qreal r = (qreal)qMin(width, height) / 2.0;
    r -= r / 50.0;
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);
    // Draw notches
    if (option->subControls & QStyle::SC_DialTickmarks) {
        painter->setPen(pal.color(QPalette::Disabled, QPalette::Text));
        painter->drawLines(calcLines(option));
    }
    const qreal d_ = r / 6;
    const qreal dx = option->rect.x() + d_ + (width - 2 * r) / 2 + 1;
    const qreal dy = option->rect.y() + d_ + (height - 2 * r) / 2 + 1;
    QRectF br = QRectF(dx + 0.5, dy + 0.5, int(r * 2 - 2 * d_ - 2), int(r * 2 - 2 * d_ - 2));
    if (enabled) {
        painter->setBrush(buttonColor);
    } else {
        painter->setBrush(Qt::NoBrush);
    }
    painter->setPen(Dc::outlineOf(option->palette));
    painter->drawEllipse(br);
    painter->setBrush(Qt::NoBrush);
    painter->setPen(Dc::specularOf(buttonColor));
    painter->drawEllipse(br.adjusted(1, 1, -1, -1));
    if (option->state & QStyle::State_HasFocus) {
        QColor highlight = pal.highlight().color();
        highlight.setHsv(highlight.hue(), qMin(160, highlight.saturation()),
                         qMax(230, highlight.value()));
        highlight.setAlpha(127);
        painter->setPen(QPen(highlight, 2.0));
        painter->setBrush(Qt::NoBrush);
        painter->drawEllipse(br.adjusted(-1, -1, 1, 1));
    }
    QPointF dp = calcRadialPos(option, qreal(0.70));
    const qreal ds = r / qreal(7.0);
    QRectF dialRect(dp.x() - ds, dp.y() - ds, 2 * ds, 2 * ds);
    painter->setBrush(option->palette.color(QPalette::Window));
    painter->setPen(Dc::outlineOf(option->palette));
    painter->drawEllipse(dialRect.adjusted(-1, -1, 1, 1));
    painter->restore();
}
#endif // QT_CONFIG(dial)

int fontMetricsWidth(const QFontMetrics &fontMetrics, const QString &text)
{
#if QT_VERSION < QT_VERSION_CHECK(5, 11, 0)
    return fontMetrics.width(text, text.size(), Qt::TextBypassShaping);
#else
    return fontMetrics.horizontalAdvance(text);
#endif
}

// This always draws the arrow with the correct aspect ratio, even if the
// provided bounding rect is non-square. The base edge of the triangle is
// snapped to a whole pixel to avoid anti-aliasing making it look soft.
//
void drawArrow(QPainter *p, QRect rect, Qt::ArrowType arrowDirection, const QBrush &brush)
{
    if (rect.isEmpty())
        return;

    const int arrowWidth = int(dpiScaled(14));
    const int arrowHeight = int(dpiScaled(8));

    const int arrowMax = qMin(arrowHeight, arrowWidth);
    const int rectMax = qMin(rect.height(), rect.width());
    const int size = qMin(arrowMax, rectMax);

    QRectF arrowRect;
    arrowRect.setWidth(size);
    arrowRect.setHeight(arrowHeight * size / arrowWidth);
    if (arrowDirection == Qt::LeftArrow || arrowDirection == Qt::RightArrow)
        arrowRect = arrowRect.transposed();
    arrowRect.moveTo(rect.left() + ((rect.width() - arrowRect.width()) / 2.0),
                     rect.top() + ((rect.height() - arrowRect.height()) / 2.0));

    QPointF points[3];
    switch (arrowDirection) {
    case Qt::DownArrow:
        arrowRect.setTop(std::round(arrowRect.top()));
        points[0] = arrowRect.topLeft();
        points[1] = arrowRect.topRight();
        points[2] = QPointF(arrowRect.center().x(), arrowRect.bottom());
        break;
    case Qt::RightArrow: {
        arrowRect.setLeft(std::round(arrowRect.left()));
        points[0] = arrowRect.topLeft();
        points[1] = arrowRect.bottomLeft();
        points[2] = QPointF(arrowRect.right(), arrowRect.center().y());
        break;
    }
    case Qt::LeftArrow:
        arrowRect.setRight(std::round(arrowRect.right()));
        points[0] = arrowRect.topRight();
        points[1] = arrowRect.bottomRight();
        points[2] = QPointF(arrowRect.left(), arrowRect.center().y());
        break;
    case Qt::UpArrow:
    default:
        arrowRect.setBottom(std::round(arrowRect.bottom()));
        points[0] = arrowRect.bottomLeft();
        points[1] = arrowRect.bottomRight();
        points[2] = QPointF(arrowRect.center().x(), arrowRect.top());
        break;
    }
    auto oldPen = p->pen();
    auto oldBrush = p->brush();
    bool oldAA = p->testRenderHint(QPainter::Antialiasing);
    p->setPen(Qt::NoPen);
    p->setBrush(brush);
    if (!oldAA) {
        p->setRenderHint(QPainter::Antialiasing);
    }
    p->drawConvexPolygon(points, 3);
    p->setPen(oldPen);
    p->setBrush(oldBrush);
    if (!oldAA) {
        p->setRenderHint(QPainter::Antialiasing, false);
    }
}

// Pass allowEnabled as false to always draw the arrow with the disabled color,
// even if the underlying palette's current color group is not disabled. Useful
// for parts of widgets which may want to be drawn as disabled even if the
// actual widget is not set as disabled, such as scrollbar step buttons when
// the scrollbar has no movable range.
Q_NEVER_INLINE void drawArrow(QPainter *painter, QRect rect, Qt::ArrowType type,
                              const PhSwatch &swatch, bool allowEnabled = true)
{
    if (rect.isEmpty())
        return;
    using namespace SwatchColors;
    QWin11Phantom::drawArrow(
            painter, rect, type,
            swatch.color(allowEnabled ? S_indicator_current : S_indicator_disabled));
}

// This draws exactly within the rect provided. If you provide a square rect,
// it will appear too wide -- you probably want to shrink the width of your
// square first by multiplying it with CheckMark_WidthOfHeightScale.
Q_NEVER_INLINE void drawCheck(QPainter *painter, QPen &scratchPen, const QRectF &r,
                              const PhSwatch &swatch, Swatchy color)
{
    using namespace QWin11Phantom::SwatchColors;

    const int arrowWidth = int(dpiScaled(14));
    const int arrowHeight = int(dpiScaled(14));

    const int arrowMax = qMin(arrowHeight, arrowWidth);
    const int rectMax = qMin(r.height(), r.width());
    const int size = qMin(arrowMax, rectMax);

    QRectF arrowRect;
    arrowRect.setWidth(size);
    arrowRect.setHeight(arrowHeight * size / arrowWidth);
    arrowRect.moveTo(r.left() + ((r.width() - arrowRect.width()) / 2.0),
                     r.top() + ((r.height() - arrowRect.height()) / 2.0));

    qreal rx, ry, rw, rh;
    QRectF(arrowRect).getRect(&rx, &ry, &rw, &rh);
    qreal penWidth = 0.20 * qMin(rw, rh);
    qreal dimx = rw - penWidth;
    qreal dimy = rh - penWidth;
    if (dimx < 0.5 || dimy < 0.5)
        return;
    qreal x = (rw - dimx) / 2 + rx;
    qreal y = (rh - dimy) / 2 + ry;
    QPointF points[3];
    points[0] = QPointF(0.0, 0.55);
    points[1] = QPointF(0.4, 1.0);
    points[2] = QPointF(1.0, 0);
    for (int i = 0; i < 3; ++i) {
        QPointF pnt = points[i];
        pnt.setX(pnt.x() * dimx + x);
        pnt.setY(pnt.y() * dimy + y);
        points[i] = pnt;
    }
    scratchPen.setBrush(swatch.color(color));
    scratchPen.setCapStyle(Qt::RoundCap);
    scratchPen.setJoinStyle(Qt::RoundJoin);
    scratchPen.setWidthF(penWidth);
    QWin11Phantom::PSave save(painter);
    if (!painter->testRenderHint(QPainter::Antialiasing))
        painter->setRenderHint(QPainter::Antialiasing);
    painter->setPen(scratchPen);
    painter->setBrush(Qt::NoBrush);
    painter->drawPolyline(points, 3);
}

Q_NEVER_INLINE void drawHyphen(QPainter *painter, QPen &scratchPen, const QRectF &r,
                               const PhSwatch &swatch, Swatchy color)
{
    using namespace QWin11Phantom::SwatchColors;
    qreal rx, ry, rw, rh;
    r.getRect(&rx, &ry, &rw, &rh);
    qreal penWidth = 0.20 * qMin(rw, rh);
    qreal dimx = rw - penWidth;
    qreal dimy = rh - penWidth;
    if (dimx < 0.5 || dimy < 0.5)
        return;
    qreal x = (rw - dimx) / 2 + rx;
    qreal y = (rh - dimy) / 2 + ry;
    QPointF p0(0.0 * dimx + x, 0.5 * dimy + y);
    QPointF p1(1.0 * dimx + x, 0.5 * dimy + y);
    scratchPen.setBrush(swatch.color(color));
    scratchPen.setCapStyle(Qt::RoundCap);
    scratchPen.setWidthF(penWidth);
    QWin11Phantom::PSave save(painter);
    if (!painter->testRenderHint(QPainter::Antialiasing))
        painter->setRenderHint(QPainter::Antialiasing);
    painter->setPen(scratchPen);
    painter->setBrush(Qt::NoBrush);
    painter->drawLine(p0, p1);
}

Q_NEVER_INLINE void drawMdiButton(QPainter *painter, const QStyleOptionTitleBar *option, QRect tmp,
                                  bool hover, bool sunken)
{
    QColor dark;
    dark.setHsv(option->palette.button().color().hue(),
                qMin(255, (int)(option->palette.button().color().saturation())),
                qMin(255, (int)(option->palette.button().color().value() * 0.7)));
    QColor highlight = option->palette.highlight().color();
    bool active = (option->titleBarState & QStyle::State_Active);
    QColor titleBarHighlight(255, 255, 255, 60);
    if (sunken)
        painter->fillRect(tmp.adjusted(1, 1, -1, -1),
                          option->palette.highlight().color().darker(120));
    else if (hover)
        painter->fillRect(tmp.adjusted(1, 1, -1, -1), QColor(255, 255, 255, 20));
    if (sunken)
        titleBarHighlight = highlight.darker(130);
    QColor mdiButtonBorderColor(active ? option->palette.highlight().color().darker(180)
                                       : dark.darker(110));
    painter->setPen(QPen(mdiButtonBorderColor));
    const QLine lines[4] = { QLine(tmp.left() + 2, tmp.top(), tmp.right() - 2, tmp.top()),
                             QLine(tmp.left() + 2, tmp.bottom(), tmp.right() - 2, tmp.bottom()),
                             QLine(tmp.left(), tmp.top() + 2, tmp.left(), tmp.bottom() - 2),
                             QLine(tmp.right(), tmp.top() + 2, tmp.right(), tmp.bottom() - 2) };
    painter->drawLines(lines, 4);
    const QPoint points[4] = { QPoint(tmp.left() + 1, tmp.top() + 1),
                               QPoint(tmp.right() - 1, tmp.top() + 1),
                               QPoint(tmp.left() + 1, tmp.bottom() - 1),
                               QPoint(tmp.right() - 1, tmp.bottom() - 1) };
    painter->drawPoints(points, 4);
    painter->setPen(titleBarHighlight);
    painter->drawLine(tmp.left() + 2, tmp.top() + 1, tmp.right() - 2, tmp.top() + 1);
    painter->drawLine(tmp.left() + 1, tmp.top() + 2, tmp.left() + 1, tmp.bottom() - 2);
}

void drawRoundedBorder(QPainter *p, const QRectF &rect, double thickness, double roundedRadius,
                       const PhSwatch &swatch, Swatchy border)
{
    PSave save(p);

    /* Draw the thin border */
    p->setPen(QPen(swatch.color(border), thickness));
    p->setBrush(Qt::NoBrush);
    p->setRenderHint(QPainter::Antialiasing);

    auto tempRect = rect.adjusted(thickness, thickness, -thickness, -thickness);
    if (roundedRadius == 0) {
        p->drawRect(tempRect);
    } else {
        p->drawRoundedRect(tempRect, roundedRadius, roundedRadius);
    }
}

void drawRoundedBorder(QPainter *p, const QRectF &rect, double roundedRadius,
                       const PhSwatch &swatch, Swatchy border)
{
    drawRoundedBorder(p, rect, QWin11Phantom::Default_Thickness, roundedRadius, swatch, border);
}

void drawDefaultRoundedBorder(QPainter *p, const QRectF &rect, const PhSwatch &swatch,
                              Swatchy border)
{
    drawRoundedBorder(p, rect, QWin11Phantom::Default_Thickness, QWin11Phantom::Default_Rounding,
                      swatch, border);
}

void drawFilledRect(QPainter *p, const QRectF &rect, double roundedRadius, double thickness,
                    const PhSwatch &swatch, Swatchy fill, Swatchy border)
{
    QPen tempPen(Qt::NoPen);
    if (thickness > 0 && border != SwatchColors::S_none) {
        tempPen = QPen(swatch.color(border), thickness);
    }

    PSave save(p);
    p->setPen(tempPen);
    p->setBrush(swatch.color(fill));
    p->setRenderHint(QPainter::Antialiasing);

    auto tempRect = rect.adjusted(thickness, thickness, -thickness, -thickness);
    if (roundedRadius == 0) {
        p->drawRect(tempRect);
    } else {
        p->drawRoundedRect(tempRect, roundedRadius, roundedRadius);
    }
}

void drawFilledRectNoBorder(QPainter *p, const QRectF &rect, double radius, const PhSwatch &swatch,
                            Swatchy fill)
{
    drawFilledRect(p, rect, radius, 0, swatch, fill, SwatchColors::S_none);
}

void fillRectEdges(QPainter *p, const QRectF &rect, Qt::Edges edges, const QMarginsF &margins,
                   double radius, const PhSwatch &swatch, Swatchy border)
{
    if (edges & Qt::LeftEdge) {
        auto r = rect;
        r.setRight(rect.left() + margins.left());
        p->setClipRect(r);

        drawFilledRectNoBorder(p, rect, radius, swatch, border);
    }
    if (edges & Qt::TopEdge) {
        auto r = rect;
        r.setBottom(rect.top() + margins.top());
        p->setClipRect(r);

        drawFilledRectNoBorder(p, rect, radius, swatch, border);
    }
    if (edges & Qt::RightEdge) {
        auto r = rect;
        r.setLeft(rect.right() - margins.right());
        p->setClipRect(r);

        drawFilledRectNoBorder(p, rect, radius, swatch, border);
    }
    if (edges & Qt::BottomEdge) {
        auto r = rect;
        r.setTop(rect.bottom() - margins.bottom());
        p->setClipRect(r);

        drawFilledRectNoBorder(p, rect, radius, swatch, border);

        //        PSave save(p);
        //        p->setPen(Qt::NoPen);
        //        p->setBrush(swatch.color(border));
        //        p->setRenderHint(QPainter::Antialiasing);

        //        QPainterPath path;
        //        path.addRoundedRect(rect, radius, radius, Qt::AbsoluteSize);

        //        QPainterPath subtractPath;
        //        QRectF subtractRect = rect;
        //        r.setBottom(rect.bottom()-10);
        //        subtractPath.addRect(subtractRect);

        //        path = path.subtracted(subtractPath);

        //        p->drawPath(path);
    }

    p->setClipRect(QRect(), Qt::ClipOperation::NoClip);
}

void fillRectEdges(QPainter *p, const QRectF &rect, Qt::Edges edges, double thickness,
                   double radius, const PhSwatch &swatch, Swatchy border)
{
    fillRectEdges(p, rect, edges, QMarginsF(thickness, thickness, thickness, thickness), radius,
                  swatch, border);
}

QRectF expandRect(const QRectF &rect, Qt::Edges edges, double delta)
{
    double l = edges & Qt::LeftEdge ? -delta : 0.0;
    double t = edges & Qt::TopEdge ? -delta : 0.0;
    double r = edges & Qt::RightEdge ? delta : 0.0;
    double b = edges & Qt::BottomEdge ? delta : 0.0;
    return rect.adjusted(l, t, r, b);
}

Qt::Edge oppositeEdge(Qt::Edge edge)
{
    switch (edge) {
    case Qt::LeftEdge:
        return Qt::RightEdge;
    case Qt::TopEdge:
        return Qt::BottomEdge;
    case Qt::RightEdge:
        return Qt::LeftEdge;
    case Qt::BottomEdge:
        return Qt::TopEdge;
    }
    return Qt::TopEdge;
}

QRectF rectTranslatedTowardEdge(const QRectF &rect, Qt::Edge edge, double delta)
{
    switch (edge) {
    case Qt::LeftEdge:
        return rect.translated(-delta, 0);
    case Qt::TopEdge:
        return rect.translated(0, -delta);
    case Qt::RightEdge:
        return rect.translated(delta, 0);
    case Qt::BottomEdge:
        return rect.translated(0, delta);
    }
    return rect;
}

QRectF rectFromInnerEdgeWithThickness(const QRectF &rect, Qt::Edge edge, double thickness)
{
    double x, y, w, h;
    rect.getRect(&x, &y, &w, &h);
    QRectF r;
    switch (edge) {
    case Qt::LeftEdge:
        r = QRectF(x, y, thickness, h);
        break;
    case Qt::TopEdge:
        r = QRectF(x, y, w, thickness);
        break;
    case Qt::RightEdge:
        r = QRectF((x + w) - thickness, y, thickness, h);
        break;
    case Qt::BottomEdge:
        r = QRectF(x, (y + h) - thickness, w, thickness);
        break;
    }
    return r & rect;
}

} // namespace
} // namespace QWin11Phantom

QWin11PhantomStylePrivate::QWin11PhantomStylePrivate() : headSwatchFastKey(0) { }

QWin11PhantomStyle::QWin11PhantomStyle() : d(new QWin11PhantomStylePrivate)
{
    setObjectName(QLatin1String("Phantom"));
}

QWin11PhantomStyle::~QWin11PhantomStyle()
{
    delete d;
}

// Draw text in a rectangle. The current pen set on the painter is used, unless
// an explicit textRole is set, in which case the palette will be used. The
// enabled bool indicates whether the text is enabled or not, and can influence
// how the text is drawn outside of just color. Wrapping and alignment flags
// can be passed in `alignment`.
void QWin11PhantomStyle::drawItemText(QPainter *painter, const QRect &rect, int alignment,
                                      const QPalette &pal, bool enabled, const QString &text,
                                      QPalette::ColorRole textRole) const
{
    Q_UNUSED(enabled);
    if (text.isEmpty())
        return;
    if (textRole == QPalette::NoRole) {
        painter->drawText(rect, alignment, text);
        return;
    }
    QPen savedPen = painter->pen();
    const QBrush &newBrush = pal.brush(textRole);
    bool changed = false;
    if (savedPen.brush() != newBrush) {
        changed = true;
        painter->setPen(QPen(newBrush, savedPen.widthF()));
    }
    painter->drawText(rect, alignment, text);
    if (changed) {
        painter->setPen(savedPen);
    }
}

void QWin11PhantomStyle::drawPrimitive(PrimitiveElement elem, const QStyleOption *option,
                                       QPainter *painter, const QWidget *widget) const
{
    Q_ASSERT(option);
    if (!option)
        return;
#ifdef BUILD_WITH_EASY_PROFILER
    EASY_BLOCK("drawPrimitive");
    const char *elemCString = QMetaEnum::fromType<QStyle::PrimitiveElement>().valueToKey(elem);
    EASY_TEXT("Element", elemCString);
#endif
    using Swatchy = QWin11Phantom::Swatchy;
    using namespace QWin11Phantom::SwatchColors;
    namespace Ph = QWin11Phantom;
    auto ph_swatchPtr =
            getCachedSwatchOfQPalette(&d->swatchCache, &d->headSwatchFastKey, option->palette);
    const Ph::PhSwatch &swatch = *ph_swatchPtr.data();
    const int state = option->state;
    // Cast to int here to suppress warnings about cases listed which are not in
    // the original enum. This is for custom primitive elements.
    switch ((int)elem) {
    case PE_Frame: {
        if (widget && widget->inherits("QComboBoxPrivateContainer")) {
            QStyleOption copy = *option;
            copy.state |= State_Raised;
            proxy()->drawPrimitive(PE_PanelMenu, &copy, painter, widget);
            break;
        }
        Ph::drawRoundedBorder(painter, option->rect, 0, swatch, S_window_outline);
        break;
    }
    case PE_FrameMenu: {
        break;
    }
    case PE_FrameDockWidget: {
        painter->save();
        QColor softshadow = option->palette.window().color().darker(120);
        QRect r = option->rect;
        painter->setPen(softshadow);
        painter->drawRect(r.adjusted(0, 0, -1, -1));
        painter->setPen(QPen(option->palette.light(), 1));
        painter->drawLine(QPoint(r.left() + 1, r.top() + 1), QPoint(r.left() + 1, r.bottom() - 1));
        painter->setPen(QPen(option->palette.window().color().darker(120)));
        painter->drawLine(QPoint(r.left() + 1, r.bottom() - 1),
                          QPoint(r.right() - 2, r.bottom() - 1));
        painter->drawLine(QPoint(r.right() - 1, r.top() + 1),
                          QPoint(r.right() - 1, r.bottom() - 1));
        painter->restore();
        break;
    }
    case PE_FrameGroupBox: {
        QRect frame = option->rect;
        Ph::PSave save(painter);
        Ph::drawRoundedBorder(painter, frame, Ph::Default_Thickness, Ph::Default_Rounding, swatch,
                              S_window_divider);
        break;
    }
    case PE_IndicatorBranch: {
        if (!(option->state & State_Children))
            break;
        Qt::ArrowType arrow;
        if (option->state & State_Open) {
            arrow = Qt::DownArrow;
        } else if (option->direction != Qt::RightToLeft) {
            arrow = Qt::RightArrow;
        } else {
            arrow = Qt::LeftArrow;
        }
        bool useSelectionColor = false;
        if (option->state & State_Selected) {
            if (auto ivopt = qstyleoption_cast<const QStyleOptionViewItem *>(option)) {
                useSelectionColor = ivopt->showDecorationSelected;
            }
        }
        Swatchy color = useSelectionColor ? S_highlightedText : S_indicator_current;
        QRect r = option->rect;
        Ph::drawArrow(painter, r, arrow, swatch.color(color));
        break;
    }
    case PE_IndicatorMenuCheckMark: {
        // For this PE, QCommonStyle treats State_On as drawing the check with the
        // highlighted text color, and otherwise with the regular text color. I
        // guess we should match that behavior, even though it's not consistent
        // with other check box/mark drawing in QStyle (buttons and item view
        // items.) QCommonStyle also doesn't care about tri-state or unchecked
        // states -- it seems that if you call this, you want a check, and nothing
        // else.
        //
        // We'll also catch State_Selected and treat it equivalently (the way you'd
        // expect.) We'll use windowText instead of text, though -- probably
        // doesn't matter.
        Swatchy fgColor = S_windowText;
        bool isSelected = option->state & (State_Selected | State_On);
        bool isEnabled = option->state & State_Enabled;
        if (isSelected) {
            fgColor = S_highlightedText;
        } else if (!isEnabled) {
            fgColor = S_windowText_disabled;
        }
        qreal rx, ry, rw, rh;
        QRectF(option->rect).getRect(&rx, &ry, &rw, &rh);
        qreal dim = qMin(rw, rh);
        const qreal insetScale = 0.6;
        qreal dimx = dim * insetScale * Ph::CheckMark_WidthOfHeightScale;
        qreal dimy = dim * insetScale;
        QRectF r_(rx + (rw - dimx) / 2, ry + (rh - dimy) / 2, dimx, dimy);
        Ph::drawCheck(painter, d->checkBox_pen_scratch, r_, swatch, fgColor);
        break;
    }
#if QT_CONFIG(itemviews)
        // Called for the content area on tree view rows that are selected
    case PE_PanelItemViewItem: {
        QCommonStyle::drawPrimitive(elem, option, painter, widget);
        break;
    }
        // Called for left-of-item-content-area on tree view rows that are selected
    case PE_PanelItemViewRow: {
        QCommonStyle::drawPrimitive(elem, option, painter, widget);
        break;
    }
#endif
#if QT_CONFIG(tabbar)
    case PE_FrameTabBarBase: {
        auto tbb = qstyleoption_cast<const QStyleOptionTabBarBase *>(option);
        if (!tbb)
            break;

        double radius = 0;
        Qt::Edge edge = Qt::TopEdge;
        switch (tbb->shape) {
        case QTabBar::RoundedNorth:
            radius = Ph::Default_Rounding;
        case QTabBar::TriangularNorth:
            edge = Qt::TopEdge;
            break;
        case QTabBar::RoundedSouth:
            radius = Ph::Default_Rounding;
        case QTabBar::TriangularSouth:
            edge = Qt::BottomEdge;
            break;
        case QTabBar::RoundedWest:
            radius = Ph::Default_Rounding;
        case QTabBar::TriangularWest:
            edge = Qt::LeftEdge;
            break;
        case QTabBar::RoundedEast:
            radius = Ph::Default_Rounding;
        case QTabBar::TriangularEast:
            edge = Qt::RightEdge;
            break;
        }
        Ph::fillRectEdges(painter, option->rect, edge, Ph::Edge_Default_Thickness, radius, swatch,
                          S_window_outline);
        break;
    }
#endif // QT_CONFIG(tabbar)
    case PE_PanelScrollAreaCorner: {
        bool isLeftToRight = option->direction != Qt::RightToLeft;
        Qt::Edges edges = Qt::TopEdge;
        QRect bgRect = option->rect;
        if (isLeftToRight) {
            edges |= Qt::LeftEdge;
            bgRect.setX(bgRect.x() + 1);
        } else {
            edges |= Qt::RightEdge;
            bgRect.setWidth(bgRect.width() - 1);
        }
        painter->fillRect(bgRect, swatch.color(S_window));
        Ph::fillRectEdges(painter, option->rect, edges, Ph::Edge_Default_Thickness, 0, swatch,
                          S_window_outline);
        break;
    }
    case PE_IndicatorArrowUp:
    case PE_IndicatorArrowDown:
    case PE_IndicatorArrowRight:
    case PE_IndicatorArrowLeft: {
        Qt::ArrowType arrow = Qt::UpArrow;
        switch (elem) {
        case PE_IndicatorArrowUp:
            arrow = Qt::UpArrow;
            break;
        case PE_IndicatorArrowDown:
            arrow = Qt::DownArrow;
            break;
        case PE_IndicatorArrowRight:
            arrow = Qt::RightArrow;
            break;
        case PE_IndicatorArrowLeft:
            arrow = Qt::LeftArrow;
            break;
        default:
            break;
        }

        Ph::drawArrow(painter, option->rect, arrow, swatch);
        break;
    }
    case PE_IndicatorItemViewItemCheck: {
        QStyleOptionButton button;
        button.QStyleOption::operator=(*option);
        button.state &= ~State_MouseOver;
        proxy()->drawPrimitive(PE_IndicatorCheckBox, &button, painter, widget);
        return;
    }
    case PE_IndicatorHeaderArrow: {
        auto header = qstyleoption_cast<const QStyleOptionHeader *>(option);
        if (!header)
            return;
        QRect r = header->rect;
        QPoint offset = QPoint(0, -1);
        if (header->sortIndicator & QStyleOptionHeader::SortUp) {
            Ph::drawArrow(painter, r.translated(offset), Qt::DownArrow, swatch);
        } else if (header->sortIndicator & QStyleOptionHeader::SortDown) {
            Ph::drawArrow(painter, r.translated(offset), Qt::UpArrow, swatch);
        }
        break;
    }
    case PE_IndicatorButtonDropDown: {
        // Temp hack until we implement CC_ToolButton: avoid double-stacked border
        // by clipping off one edge slightly.
        QStyleOption opt0 = *option;
        if (opt0.direction != Qt::RightToLeft) {
            opt0.rect.adjust(-1, 0, 0, 0);
        } else {
            opt0.rect.adjust(0, 0, 1, 0);
        }
        proxy()->drawPrimitive(PE_PanelButtonTool, &opt0, painter, widget);
        break;
    }

    case PE_IndicatorToolBarSeparator: {
        QRectF r = option->rect;
        if (option->state & State_Horizontal) {
            if (r.height() >= 10)
                r.adjust(0, 3, 0, -3);
            r.setWidth(r.width() / 2 + Ph::Default_Thickness);
            Ph::fillRectEdges(painter, r, Qt::RightEdge, Ph::Default_Thickness, 0, swatch,
                              S_window_divider);
        } else {
            // TODO replace with new code
            const int margin = 6;
            const int offset = r.height() / 2;
            painter->setPen(QPen(option->palette.window().color().darker(110)));
            painter->drawLine(r.topLeft().x() + margin, r.topLeft().y() + offset,
                              r.topRight().x() - margin, r.topRight().y() + offset);
            painter->setPen(QPen(option->palette.window().color().lighter(110)));
            painter->drawLine(r.topLeft().x() + margin, r.topLeft().y() + offset + 1,
                              r.topRight().x() - margin, r.topRight().y() + offset + 1);
        }
        break;
    }
    case PE_PanelButtonTool: {
        bool isDown = option->state & State_Sunken;
        bool isOn = option->state & State_On;
        bool hasFocus =
                (option->state & State_HasFocus && option->state & State_KeyboardFocusChange);
        Swatchy outline = S_window_outline;
        Swatchy fill = S_button;
        if (isDown) {
            fill = S_button_pressed;
        } else if (isOn) {
            // kinda repurposing this, hmm
            fill = S_button_on;
        }
        if (hasFocus) {
            outline = S_highlight_outline;
        }
        QRectF r = option->rect;
        Ph::PSave save(painter);
        Ph::drawFilledRect(painter, r, Ph::Default_Rounding, Ph::Default_Thickness, swatch, fill,
                           outline);
        break;
    }
    case PE_IndicatorDockWidgetResizeHandle: {
        QStyleOption dockWidgetHandle = *option;
        bool horizontal = option->state & State_Horizontal;
        dockWidgetHandle.state.setFlag(State_Horizontal, !horizontal);
        proxy()->drawControl(CE_Splitter, &dockWidgetHandle, painter, widget);
        break;
    }
    case PE_FrameWindow: {
        break;
    }
    case PE_FrameLineEdit: {
        QRect r = option->rect;

        drawRoundedBorder(painter, r, Ph::Default_Rounding, swatch, S_window_outline);
        if (option->state & State_HasFocus) {
            Ph::fillRectEdges(painter, r, Qt::BottomEdge, Ph::Frame_Selected_Edge_Width,
                              Ph::Default_Rounding, swatch, S_highlight_outline);
        } else {
            Ph::fillRectEdges(painter, r, Qt::BottomEdge, Ph::Edge_Default_Thickness,
                              Ph::Default_Rounding, swatch, S_window_outline);
        }
        break;
    }
    case PE_PanelLineEdit: {
        auto panel = qstyleoption_cast<const QStyleOptionFrame *>(option);
        if (!panel)
            break;

        if (widget) {
            const auto *spinBox = dynamic_cast<const QAbstractSpinBox *>(widget->parentWidget());
            if (spinBox) {
                return;
            }

            const auto *comboBox = dynamic_cast<const QComboBox *>(widget->parentWidget());
            if (comboBox) {
                return;
            }
        }

        // We intentionally don't inset the fill rect, even if the frame will paint
        // over the perimeter, because an inset with rounding enabled may cause
        // some miscolored separated pixels between the fill and the border, since
        // we're forced to paint them in two separate draw calls.

        Swatchy fill = S_button;
        if (option->state & QStyle::State_HasFocus) {
            fill = S_base;
        } else if (option->state & QStyle::State_MouseOver) {
            fill = S_button_hover;
        }

        Ph::drawFilledRectNoBorder(painter, option->rect, Ph::Default_Rounding, swatch, fill);

        if (panel->lineWidth > 0)
            proxy()->drawPrimitive(PE_FrameLineEdit, option, painter, widget);
        break;
    }
    case PE_IndicatorCheckBox: {
        auto checkbox = qstyleoption_cast<const QStyleOptionButton *>(option);
        if (!checkbox)
            break;
        QRect r = option->rect;
        bool isHighlighted =
                option->state & State_HasFocus && option->state & State_KeyboardFocusChange;
        bool isSelected = option->state & State_Selected;
        bool isFlat = checkbox->features & QStyleOptionButton::Flat;
        bool isEnabled = option->state & State_Enabled;
        bool isPressed = state & State_Sunken;
        Swatchy outlineColor = isHighlighted ? S_highlight_outline : S_window_outline;
        Swatchy bgFillColor = isPressed ? S_highlight : S_base;
        Swatchy fgColor = isFlat ? S_windowText : S_text;
        if (isPressed && !isFlat) {
            fgColor = S_highlightedText;
        }
        // Bare checkmarks that are selected should draw with the highlighted text
        // color.
        if (isSelected && isFlat) {
            fgColor = S_highlightedText;
        }
        if (!isFlat) {
            Ph::drawFilledRect(painter, r, Ph::Default_Rounding, Ph::Default_Thickness, swatch,
                               bgFillColor, outlineColor);
            if (isEnabled) {
                Ph::fillRectEdges(painter, r, Qt::BottomEdge, Ph::Edge_Default_Thickness,
                                  Ph::Default_Rounding, swatch, outlineColor);
            }
        }

        qreal rx, ry, rw, rh;
        QRectF(r.adjusted(2, 2, -2, -2)).getRect(&rx, &ry, &rw, &rh);

        if (checkbox->state & State_NoChange) {
            const qreal insetScale = 0.8;
            qreal dimx = rw * insetScale;
            qreal dimy = rh * insetScale;
            QRectF r_(rx + (rw - dimx) / 2, ry + (rh - dimy) / 2, dimx, dimy);
            Ph::drawHyphen(painter, d->checkBox_pen_scratch, r_, swatch, fgColor);
        } else if (checkbox->state & State_On) {
            const qreal insetScale = 0.8;
            // kinda wrong, assumes we're already square, but we probably are
            qreal dimx = rw * insetScale * Ph::CheckMark_WidthOfHeightScale;
            qreal dimy = rh * insetScale;
            QRectF r_(rx + (rw - dimx) / 2, ry + (rh - dimy) / 2, dimx, dimy);
            Ph::drawCheck(painter, d->checkBox_pen_scratch, r_, swatch, fgColor);
        }
        break;
    }
    case PE_IndicatorRadioButton: {
        qreal rx, ry, rw, rh;
        QRectF(option->rect).getRect(&rx, &ry, &rw, &rh);
        bool isHighlighted =
                option->state & State_HasFocus && option->state & State_KeyboardFocusChange;
        bool isSunken = state & State_Sunken;
        Swatchy outlineColor = isHighlighted ? S_highlight_outline : S_window_outline;
        Swatchy bgFillColor = isSunken ? S_highlight : S_base;
        QPointF circleCenter(rx + rw / 2.0, ry + rh / 2.0);
        const qreal lineThickness = 1.0;
        qreal outlineRadius = (qMin(rw, rh) - lineThickness) / 2.0;
        Ph::PSave save(painter);
        painter->setRenderHint(QPainter::Antialiasing);
        painter->setBrush(swatch.color(bgFillColor));
        painter->setPen(swatch.pen(outlineColor));
        painter->drawEllipse(circleCenter, outlineRadius, outlineRadius);

        if (state & State_On) {
            Swatchy fgColor = isSunken ? S_highlightedText : S_windowText;
            qreal checkmarkRadius = outlineRadius / 2.32;
            painter->setPen(Qt::NoPen);
            painter->setBrush(swatch.color(fgColor));
            painter->drawEllipse(circleCenter, checkmarkRadius, checkmarkRadius);
        }
        break;
    }
    case PE_IndicatorToolBarHandle: {
        if (!option)
            break;
        QRect r = option->rect;
        if (r.width() < 3 || r.height() < 3)
            break;
        int rows = 3;
        int columns = 2;
        if (option->state & State_Horizontal) {
        } else {
            qSwap(columns, rows);
        }
        int dotLen = (int)Ph::dpiScaled(2);
        QSize occupied(dotLen * (columns * 2 - 1), dotLen * (rows * 2 - 1));
        QRect rr = QStyle::alignedRect(option->direction, Qt::AlignCenter, QSize(occupied), r);
        int x = rr.x();
        int y = rr.y();
        for (int row = 0; row < rows; ++row) {
            for (int col = 0; col < columns; ++col) {
                int x_ = x + col * 2 * dotLen;
                int y_ = y + row * 2 * dotLen;
                painter->fillRect(x_, y_, dotLen, dotLen, swatch.color(S_window_divider));
            }
        }
        break;
    }
    case PE_FrameDefaultButton:
        break;
    case PE_FrameFocusRect: {
        auto fropt = qstyleoption_cast<const QStyleOptionFocusRect *>(option);
        if (!fropt)
            break;
        // ### check for d->alt_down
        if (!(fropt->state & State_KeyboardFocusChange))
            return;
#if QT_CONFIG(itemviews)
        if (fropt->state & State_Item) {
            if (auto itemView = qobject_cast<const QAbstractItemView *>(widget)) {
                // TODO either our grid line hack is interfering, or Qt has a bug, but
                // in RTL layout the grid borders can leave junk behind in the grid
                // areas and the right edge of the focus rect may not get painted.
                // (Sometimes it will, though.) To replicate, set to RTL mode, and move
                // the current around in a table view without the selection being on
                // the current.
                if (option->state & QStyle::State_Selected) {
                    bool showCurrent = true;
                    bool hasTableGrid = false;
                    const auto selectionMode = itemView->selectionMode();
                    if (selectionMode == QAbstractItemView::SingleSelection) {
                        showCurrent = false;
                    } else {
                        // Table views will can have a "current" frame drawn even if the
                        // "current" is within the selected range. Other item views won't,
                        // which means the "current" frame will be invisible if it's on a
                        // selected item. This is a compromise between the broken drawing
                        // behavior of Qt item views of drawing "current" frames when they
                        // don't make sense (like a tree view where you can only select
                        // entire rows, but Qt will the frame rect around whatever column
                        // was last clicked on by the mouse, but using keyboard navigation
                        // has no effect) and not drawing them at all.
                        bool isTableView = false;
                        if (auto tableView = qobject_cast<const QTableView *>(itemView)) {
                            hasTableGrid = tableView->showGrid();
                            isTableView = true;
                        }
                        const auto selectionModel = itemView->selectionModel();
                        if (selectionModel) {
                            const auto selection = selectionModel->selection();
                            if (selection.count() == 1) {
                                const auto &range = selection.at(0);
                                if (isTableView) {
                                    // For table views, we don't draw the "current" frame if
                                    // there is exactly one cell selected and the "current" is
                                    // that cell, or if there is exactly one row or one column
                                    // selected with the behavior set to the corresponding
                                    // selection, and the "current" is that one row or column.
                                    const auto selectionBehavior = itemView->selectionBehavior();
                                    if ((range.width() == 1 && range.height() == 1)
                                        || (selectionBehavior == QAbstractItemView::SelectRows
                                            && range.height() == 1)
                                        || (selectionBehavior == QAbstractItemView::SelectColumns
                                            && range.width() == 1)) {
                                        showCurrent = false;
                                    }
                                } else {
                                    // For any other type of item view, don't draw the "current"
                                    // frame if there is a single contiguous selection, and the
                                    // "current" is within that selection. If there's a
                                    // discontiguous selection, that means the user is probably
                                    // doing something more advanced, and we should just draw the
                                    // focus frame, even if Qt might be doing it badly in some
                                    // cases.
                                    showCurrent = false;
                                }
                            }
                        }
                    }
                    if (showCurrent) {
                        // TODO handle dark-highlight-light-text
                        const double thickness = hasTableGrid ? 2.0 : 1.0;
                        Ph::drawRoundedBorder(painter, option->rect, thickness,
                                              Ph::Default_Rounding, swatch,
                                              S_itemView_multiSelection_currentBorder);
                    }
                } else {
                    Ph::drawDefaultRoundedBorder(painter, option->rect, swatch,
                                                 S_highlight_outline);
                }
                break;
            }
        }
        // It would be nice to also handle QTreeView's allColumnsShowFocus thing in
        // the above code, in addition to the normal cases for focus rects in item
        // views. Unfortunately, with allColumnsShowFocus set to true,
        // QTreeView::drawRow() calls the style to paint with PE_FrameFocusRect for
        // the row frame with the widget set to nullptr. This makes it basically
        // impossible to figure out that we need to draw a special frame for it.
        // So, if any application code is using that mode in a QTreeView, it won't
        // get special item view frames. Too bad.
#endif
        Ph::PSave save(painter);
        Ph::drawDefaultRoundedBorder(painter, option->rect, swatch, S_highlight_outline);
        break;
    }
    case PE_PanelButtonCommand:
    case PE_PanelButtonBevel: {
        bool isDefault = false;
        bool isFlat = false;
        bool isDown = option->state & State_Sunken;
        bool isOn = option->state & State_On;
        bool isMouseOver = option->state & State_MouseOver;

        if (auto button = qstyleoption_cast<const QStyleOptionButton *>(option)) {
            isDefault = (button->features & QStyleOptionButton::DefaultButton)
                    && (button->state & State_Enabled);
            isFlat = (button->features & QStyleOptionButton::Flat);
        }
        if (isFlat && !isDown && !isOn && !isMouseOver)
            break;
        bool isEnabled = option->state & State_Enabled;
        Q_UNUSED(isEnabled);
        bool hasFocus =
                (option->state & State_HasFocus && option->state & State_KeyboardFocusChange);
        Swatchy fill = S_button;
        if (isDown) {
            fill = S_button_pressed;
        } else if (isOn) {
            fill = S_button_on;
        } else if (isMouseOver) {
            fill = S_button_hover;
        }

        Swatchy outline = S_none;
        if (!isFlat) {
            outline = S_window_outline;
            if (hasFocus || isDefault) {
                outline = S_highlight_outline;
            }
        }

        QRectF r = option->rect;
        Ph::PSave save(painter);

        Ph::drawFilledRect(painter, r, Ph::Default_Rounding, Ph::Default_Thickness, swatch, fill,
                           outline);
        Ph::fillRectEdges(painter, r, Qt::BottomEdge, Ph::Edge_Default_Thickness,
                          Ph::Default_Rounding, swatch, outline);
        break;
    }
    case PE_FrameTabWidget: {
        Ph::drawFilledRect(painter, option->rect, 0, 0, swatch, S_tabFrame, S_none);
#if QT_CONFIG(tabwidget)
        auto twf = qstyleoption_cast<const QStyleOptionTabWidgetFrame *>(option);
        if (!twf)
            break;
        Ph::drawRoundedBorder(painter, option->rect, Ph::Default_Thickness, 0, swatch,
                              S_window_outline);
#endif // QT_CONFIG(tabwidget)
        break;
    }
    case PE_FrameStatusBarItem:
        break;
    case PE_IndicatorTabClose:
    case PE_PanelMenu: {
        Ph::drawFilledRect(painter, option->rect, Ph::Default_Rounding, Ph::Default_Thickness,
                           swatch, S_window, S_window_divider);
        break;
    }
    default:
        QCommonStyle::drawPrimitive(elem, option, painter, widget);
        break;
    }
}

void QWin11PhantomStyle::drawControl(ControlElement element, const QStyleOption *option,
                                     QPainter *painter, const QWidget *widget) const
{
#ifdef BUILD_WITH_EASY_PROFILER
    EASY_BLOCK("drawControl");
    const char *elemCString = QMetaEnum::fromType<QStyle::ControlElement>().valueToKey(element);
    EASY_TEXT("Element", elemCString);
#endif
    using Swatchy = QWin11Phantom::Swatchy;
    using namespace QWin11Phantom::SwatchColors;
    namespace Ph = QWin11Phantom;
    auto ph_swatchPtr =
            Ph::getCachedSwatchOfQPalette(&d->swatchCache, &d->headSwatchFastKey, option->palette);
    const Ph::PhSwatch &swatch = *ph_swatchPtr.data();

    switch (element) {
    case CE_CheckBox: {
        QCommonStyle::drawControl(element, option, painter, widget);
        // painter->fillRect(option->rect, QColor(255, 0, 0, 90));
        break;
    }
    case CE_ComboBoxLabel: {
        auto cb = qstyleoption_cast<const QStyleOptionComboBox *>(option);
        if (!cb)
            break;
        QRect editRect = proxy()->subControlRect(CC_ComboBox, cb, SC_ComboBoxEditField, widget);
        painter->save();
        painter->setClipRect(editRect);
        if (!cb->currentIcon.isNull()) {
            QIcon::Mode mode = cb->state & State_Enabled ? QIcon::Normal : QIcon::Disabled;
            QPixmap pixmap = cb->currentIcon.pixmap(cb->iconSize, mode);
            QRect iconRect(editRect);
            iconRect.setWidth(cb->iconSize.width() + 4);
            iconRect = alignedRect(cb->direction, Qt::AlignLeft | Qt::AlignVCenter, iconRect.size(),
                                   editRect);
            proxy()->drawItemPixmap(painter, iconRect, Qt::AlignCenter, pixmap);

            if (cb->direction == Qt::RightToLeft)
                editRect.translate(-4 - cb->iconSize.width(), 0);
            else
                editRect.translate(cb->iconSize.width() + 4, 0);
        }
        if (!cb->currentText.isEmpty() && !cb->editable) {
            proxy()->drawItemText(painter, editRect.adjusted(1, 0, -1, 0),
                                  visualAlignment(cb->direction, Qt::AlignLeft | Qt::AlignVCenter),
                                  cb->palette, cb->state & State_Enabled, cb->currentText,
                                  cb->editable ? QPalette::Text : QPalette::ButtonText);
        }
        painter->restore();
        break;
    }
#if QT_CONFIG(splitter)
    case CE_Splitter: {
        QRectF r = option->rect;
        // We don't have anything useful to draw if it's too thin
        if (r.width() < 5 || r.height() < 5)
            break;
        double length = Ph::dpiScaled(Ph::SplitterMaxLength);
        double thickness = Ph::dpiScaled(1);
        QSizeF size;
        if (option->state & State_Horizontal) {
            if (r.height() < length)
                length = r.height();
            size = QSizeF(thickness, length);
        } else {
            if (r.width() < length)
                length = r.width();
            size = QSizeF(length, thickness);
        }
        QRectF filledRect = QStyle::alignedRect(option->direction, Qt::AlignCenter, size.toSize(),
                                                r.toAlignedRect());
        Ph::drawFilledRect(painter, filledRect, Ph::Default_Rounding, Ph::Default_Thickness, swatch,
                           S_button_specular, S_window_divider);
        break;
    }
#endif // QT_CONFIG(splitter)
#if QT_CONFIG(rubberband)
        // TODO update this for phantom
    case CE_RubberBand: {
        if (!qstyleoption_cast<const QStyleOptionRubberBand *>(option))
            break;
        QColor highlight = option->palette.color(QPalette::Active, QPalette::Highlight);
        painter->save();
        QColor penColor = highlight.darker(120);
        penColor.setAlpha(180);
        painter->setPen(penColor);
        QColor dimHighlight(qMin(highlight.red() / 2 + 110, 255),
                            qMin(highlight.green() / 2 + 110, 255),
                            qMin(highlight.blue() / 2 + 110, 255));
        dimHighlight.setAlpha(widget && widget->isWindow() ? 255 : 80);
        painter->setRenderHint(QPainter::Antialiasing, true);
        painter->translate(0.5, 0.5);
        painter->setBrush(dimHighlight);
        painter->drawRoundedRect(option->rect.adjusted(0, 0, -1, -1), 1, 1);
        QColor innerLine = Qt::white;
        innerLine.setAlpha(40);
        painter->setPen(innerLine);
        painter->drawRoundedRect(option->rect.adjusted(1, 1, -2, -2), 1, 1);
        painter->restore();
        break;
    }
#endif // QT_CONFIG(rubberband)
    case CE_SizeGrip: {
        Qt::LayoutDirection dir = option->direction;
        QRect rect = option->rect;
        int rcx = rect.center().x();
        int rcy = rect.center().y();
        // draw grips
        for (int i = -6; i < 12; i += 3) {
            for (int j = -6; j < 12; j += 3) {
                if ((dir == Qt::LeftToRight && i > -j) || (dir == Qt::RightToLeft && j > i)) {
                    painter->fillRect(rcx + i, rcy + j, 2, 2, swatch.color(S_window_lighter));
                    painter->fillRect(rcx + i, rcy + j, 1, 1, swatch.color(S_window_darker));
                }
            }
        }
        break;
    }
#if QT_CONFIG(toolbar)
    case CE_ToolBar: {
        auto toolBar = qstyleoption_cast<const QStyleOptionToolBar *>(option);
        if (!toolBar)
            break;
        painter->fillRect(option->rect, option->palette.window().color());
        bool isFloating = false;
        if (auto tb = qobject_cast<const QToolBar *>(widget)) {
            isFloating = tb->isFloating();
        }
        if (isFloating) {
            Ph::drawDefaultRoundedBorder(painter, option->rect, swatch, S_window_outline);
        }
        break;
    }
#endif // QT_CONFIG(toolbar)
    case CE_DockWidgetTitle: {
        auto dwOpt = qstyleoption_cast<const QStyleOptionDockWidget *>(option);
        if (!dwOpt)
            break;
        painter->save();
        bool verticalTitleBar = dwOpt->verticalTitleBar;

        QRect titleRect = subElementRect(SE_DockWidgetTitleBarText, option, widget);
        if (verticalTitleBar) {
            QRect r = dwOpt->rect;
            QRect rtrans = r.transposed();
            titleRect = QRect(rtrans.left() + r.bottom() - titleRect.bottom(),
                              rtrans.top() + titleRect.left() - r.left(), titleRect.height(),
                              titleRect.width());
            painter->translate(rtrans.left(), rtrans.top() + rtrans.width());
            painter->rotate(-90);
            painter->translate(-rtrans.left(), -rtrans.top());
        }
        if (!dwOpt->title.isEmpty()) {
            QString titleText = painter->fontMetrics().elidedText(dwOpt->title, Qt::ElideRight,
                                                                  titleRect.width());
            proxy()->drawItemText(
                    painter, titleRect, Qt::AlignLeft | Qt::AlignVCenter | Qt::TextShowMnemonic,
                    dwOpt->palette, dwOpt->state & State_Enabled, titleText, QPalette::WindowText);
        }
        painter->restore();
        break;
    }
    case CE_HeaderSection: {
        auto header = qstyleoption_cast<const QStyleOptionHeader *>(option);
        if (!header)
            break;
        QRect rect = header->rect;
        Qt::Orientation orientation = header->orientation;
        QStyleOptionHeader::SectionPosition position = header->position;
        // See the "Table header layout reference" comment block at the bottom of
        // this file for more information to help understand what's going on.
        bool isLeftToRight = header->direction != Qt::RightToLeft;
        bool isHorizontal = orientation == Qt::Horizontal;
        bool isVertical = orientation == Qt::Vertical;
        bool isEnd = position == QStyleOptionHeader::End;
        bool isBegin = position == QStyleOptionHeader::Beginning;
        bool isOnlyOne = position == QStyleOptionHeader::OnlyOneSection;
        Qt::Edges edges;
        bool spansToEnd = false;
        bool isSpecialCorner = false;
#if QT_CONFIG(itemviews)
        if ((isHorizontal && isLeftToRight && isEnd) || (isHorizontal && !isLeftToRight && isBegin)
            || (isVertical && isEnd) || isOnlyOne) {
            auto hv = qobject_cast<const QHeaderView *>(widget);
            if (hv) {
                spansToEnd = hv->stretchLastSection();
                // In the case where the header item is not stretched to the end, but
                // could plausibly be in a position where it could happen to be exactly
                // the right width or height to be appear to be stretched to the end,
                // we'll check to see if it actually does exactly meet the right (or
                // bottom in vertical, or left in RTL) edge, and omit drawing the edge
                // if that's the case. This can commonly happen if you have a tree or
                // list view and don't set it to stretch, but the widget is still sized
                // exactly to hold the one column. (It could also happen if there's
                // user code running to manually stretch the last section as
                // necessary.)
                if (!spansToEnd) {
                    QRect viewBound = hv->contentsRect();
                    if (isHorizontal) {
                        if (isLeftToRight) {
                            spansToEnd = rect.right() == viewBound.right();
                        } else {
                            spansToEnd = rect.left() == viewBound.left();
                        }
                    } else if (isVertical) {
                        spansToEnd = rect.bottom() == viewBound.bottom();
                    }
                }
            } else {
                // We only need to do this check in RTL, because the corner button in
                // RTL *doesn't* need hacks applied. In LTR, we can just treat the
                // corner button like anything else on the horizontal header bar, and
                // can skip doing this inherits check.
                spansToEnd = true;

                if (isOnlyOne && !isLeftToRight && widget
                    && widget->inherits("QTableCornerButton")) {
                    isSpecialCorner = true;
                }
            }
        }
#endif

        if (isSpecialCorner) {
            // In RTL layout, the corner button in a table view doesn't have any
            // offset problems. This branch we're on is only taken if we're in RTL
            // layout and this is the corner button being drawn.
            edges |= Qt::BottomEdge;
            if (isLeftToRight)
                edges |= Qt::RightEdge;
            else
                edges |= Qt::LeftEdge;
        } else if (isHorizontal) {
            // This branch is taken for horizontal headers in either layout direction
            // or for the corner button in LTR.
            edges |= Qt::BottomEdge;
            if (isLeftToRight) {
                // In LTR, this code path may be for both the corner button *and* the
                // actual header item. It doesn't matter in this case, and we were able
                // to avoid doing an extra inherits call earlier.
                if (!spansToEnd) {
                    edges |= Qt::RightEdge;
                }
            } else {
                // Note: in right-to-left layouts for horizontal headers, the header
                // view will unfortunately be shifted to the right by 1 pixel, due to
                // what appears to be a Qt bug. This causes the vertical lines we draw
                // in the header view to misalign with the grid, and causes the
                // rightmost section to have its right edge clipped off. Therefore,
                // we'll draw the separator on the on the right edge instead of the
                // left edge. (We would have expected to draw it on the left edge in
                // RTL layout.) This makes it line up with the grid again, except for
                // the last section. right by 1 pixel.
                //
                // In RTL, the "Begin" position is on the left side for some reason
                // (the same as LTR.) So "End" is always on the right. Ok, whatever.
                // See the table at the bottom of this file if you're confused.
                if (!isOnlyOne && !isEnd) {
                    edges |= Qt::RightEdge;
                }
                // The leftmost section in RTL has to draw on both its right and left
                // edges, instead of just 1 edge like every other configuration. The
                // left edge will be offset by 1 pixel from the grid, but it's the best
                // we can do.
                if (isBegin && !spansToEnd) {
                    edges |= Qt::LeftEdge;
                }
            }
        } else if (isVertical) {
            if (isLeftToRight) {
                edges |= Qt::RightEdge;
            } else {
                edges |= Qt::LeftEdge;
            }
            if (!spansToEnd) {
                edges |= Qt::BottomEdge;
            }
        }

        painter->fillRect(rect, swatch.color(S_base));
        //        Ph::fillRectEdges(painter, rect, edges, Ph::Default_Thickness, 0, swatch,
        //        S_window_outline);
        break;
    }
    case CE_HeaderLabel: {
        auto header = qstyleoption_cast<const QStyleOptionHeader *>(option);
        if (!header)
            break;
        QRect rect = header->rect;
        if (!header->icon.isNull()) {
            int iconExtent = qMin(qMin(rect.height(), rect.width()), option->fontMetrics.height());
            auto window = widget ? widget->window()->windowHandle() : nullptr;
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
            QPixmap pixmap = header->icon.pixmap(QSize(iconExtent, iconExtent), 1.0,
                                                 (header->state & State_Enabled) ? QIcon::Normal
                                                                                 : QIcon::Disabled);
#else
            QPixmap pixmap = header->icon.pixmap(window, QSize(iconExtent, iconExtent),
                                                 (header->state & State_Enabled) ? QIcon::Normal
                                                                                 : QIcon::Disabled);
#endif
            int pixw = (int)((qreal)pixmap.width() / pixmap.devicePixelRatio());
            QRect aligned = alignedRect(header->direction, QFlag(header->iconAlignment),
                                        pixmap.size() / pixmap.devicePixelRatio(), rect);
            QRect inter = aligned.intersected(rect);
            painter->drawPixmap(inter.x(), inter.y(), pixmap, inter.x() - aligned.x(),
                                inter.y() - aligned.y(),
                                (int)(aligned.width() * pixmap.devicePixelRatio()),
                                (int)(pixmap.height() * pixmap.devicePixelRatio()));
            int margin = proxy()->pixelMetric(QStyle::PM_HeaderMargin, option, widget);
            if (header->direction == Qt::LeftToRight)
                rect.setLeft(rect.left() + pixw + margin);
            else
                rect.setRight(rect.right() - pixw - margin);
        }
        proxy()->drawItemText(painter, rect, header->textAlignment, header->palette,
                              (header->state & State_Enabled), header->text, QPalette::ButtonText);

        // But we still need some kind of indicator, so draw a line
        bool drawHighlightLine = option->state & State_On;
#if QT_CONFIG(itemviews)
        // Special logic: if the selection mode of the item view is to select every
        // row or every column, there's no real need to draw special "this
        // row/column is selected" highlight indicators in the header view. The
        // application programmer can also disable this explicitly on the header
        // view, but it's nice to have it done automatically, I think.
        if (drawHighlightLine) {
            const QAbstractItemView *itemview = nullptr;
            // Header view itself is an item view, and we don't care about its
            // selection behavior -- we care about the actual item view. So try to
            // get the widget as the header first, then find the item view from
            // there.
            auto headerview = qobject_cast<const QHeaderView *>(widget);
            if (headerview) {
                // Also don't care about highlights if there's only one row or column.
                drawHighlightLine = headerview->count() > 1;
                itemview = qobject_cast<const QAbstractItemView *>(headerview->parentWidget());
            }
            if (drawHighlightLine && itemview) {
                auto selBehavior = itemview->selectionBehavior();
                if (selBehavior == QAbstractItemView::SelectRows
                    && header->orientation == Qt::Horizontal)
                    drawHighlightLine = false;
                else if (selBehavior == QAbstractItemView::SelectColumns
                         && header->orientation == Qt::Vertical)
                    drawHighlightLine = false;
            }
        }
#endif
        if (drawHighlightLine) {
            QRectF r = option->rect;
            Qt::Edge edge;
            if (header->orientation == Qt::Horizontal) {
                edge = Qt::BottomEdge;
                r.adjust(-2, 1, 1, 1);
            } else {
                bool isLeftToRight = option->direction != Qt::RightToLeft;
                if (isLeftToRight) {
                    edge = Qt::RightEdge;
                    r.adjust(1, -2, 1, 1);
                } else {
                    edge = Qt::LeftEdge;
                    r.adjust(-1, -2, -1, 1);
                }
            }

            Ph::fillRectEdges(painter, r, edge, Ph::Edge_Default_Thickness, 0, swatch,
                              S_itemView_headerOnLine);
        }
        break;
    }
    case CE_ProgressBarGroove: {
        auto bar = qstyleoption_cast<const QStyleOptionProgressBar *>(option);
        if (!bar)
            break;

        QRect rect = Ph::progressBarRect(bar);
        Ph::PSave save(painter);
        Ph::drawFilledRect(painter, rect, Ph::Default_Rounding, Ph::Default_Thickness, swatch,
                           S_base, S_window_outline);
        save.restore();
        break;
    }
    case CE_ProgressBarContents: {
        auto bar = qstyleoption_cast<const QStyleOptionProgressBar *>(option);
        if (!bar)
            break;
        QRect filled, nonFilled;
        bool isIndeterminate = false;
        Ph::progressBarFillRects(bar, filled, nonFilled, isIndeterminate);
        if (isIndeterminate || bar->progress > bar->minimum) {
            Ph::drawFilledRectNoBorder(painter, filled, Ph::Default_Rounding, swatch,
                                       S_progressBar);
        }
        break;
    }
    case CE_ProgressBarLabel: {
        auto bar = qstyleoption_cast<const QStyleOptionProgressBar *>(option);
        if (!bar)
            break;
        if (bar->text.isEmpty())
            break;
        QRect r = bar->rect.adjusted(2, 2, -2, -2);
        if (r.isEmpty() || !r.isValid())
            break;
        QSize textSize = option->fontMetrics.size(Qt::TextSingleLine, bar->text);
        QRect textRect =
                QStyle::alignedRect(option->direction, Qt::AlignCenter, textSize, option->rect);
        textRect &= r;
        if (textRect.isEmpty())
            break;
        QRect filled, nonFilled;
        bool isIndeterminate = false;
        Ph::progressBarFillRects(bar, filled, nonFilled, isIndeterminate);
        QRect textNonFilledR = textRect & nonFilled;
        QRect textFilledR = textRect & filled;
        bool needsNonFilled = !textNonFilledR.isEmpty();
        bool needsFilled = !textFilledR.isEmpty();
        bool needsMasking = needsNonFilled && needsFilled;
        Ph::PSave save(painter);
        if (needsNonFilled) {
            if (needsMasking) {
                painter->save();
                painter->setClipRect(textNonFilledR);
            }
            painter->setPen(swatch.pen(S_text));
            painter->setBrush(Qt::NoBrush);
            painter->drawText(textRect, bar->text, Qt::AlignHCenter | Qt::AlignVCenter);
            if (needsMasking) {
                painter->restore();
            }
        }
        if (needsFilled) {
            if (needsMasking) {
                painter->save();
                painter->setClipRect(textFilledR);
            }
            painter->setPen(swatch.pen(S_highlightedText));
            painter->setBrush(Qt::NoBrush);
            painter->drawText(textRect, bar->text, Qt::AlignHCenter | Qt::AlignVCenter);
            if (needsMasking) {
                painter->restore();
            }
        }
        break;
    }
#if QT_CONFIG(menubar)
    case CE_MenuBarItem: {
        auto mbi = qstyleoption_cast<const QStyleOptionMenuItem *>(option);
        if (!mbi)
            break;
        const QRect r = option->rect;
        QRect textRect = r;
        textRect.setY(textRect.y() + (r.height() - option->fontMetrics.height()) / 2);
        int alignment = Qt::AlignHCenter | Qt::AlignTop | Qt::TextShowMnemonic | Qt::TextDontClip
                | Qt::TextSingleLine;
        if (!proxy()->styleHint(SH_UnderlineShortcut, mbi, widget))
            alignment |= Qt::TextHideMnemonic;
        const auto itemState = mbi->state;
        bool maybeHasAltKeyNavFocus = itemState & State_Selected && itemState & State_HasFocus;
        bool isSelected = itemState & State_Selected && itemState & State_Sunken;
        if (!isSelected && maybeHasAltKeyNavFocus && widget) {
            isSelected = widget->hasFocus();
        }
        Swatchy fill = isSelected ? S_highlight : S_window;
        painter->fillRect(r, swatch.color(fill));
        QPalette::ColorRole textRole = isSelected ? QPalette::HighlightedText : QPalette::Text;
        proxy()->drawItemText(painter, textRect, alignment, mbi->palette,
                              mbi->state & State_Enabled, mbi->text, textRole);
        if (isSelected)
            break;
        if (QWin11Phantom::hasTweakTrue(widget, QWin11Phantom::Tweak::menubar_no_ruler))
            break;
        if (!isSelected) {
            Ph::fillRectEdges(painter, r, Qt::BottomEdge, Ph::Edge_Default_Thickness, 0, swatch,
                              S_window_divider);
        }
        break;
    }
#endif
#if QT_CONFIG(menu)
    case CE_MenuItem: {
        auto menuItem = qstyleoption_cast<const QStyleOptionMenuItem *>(option);
        if (!menuItem)
            break;
        const auto metrics = Ph::MenuItemMetrics::ofFontHeight(option->fontMetrics.height());
        // Draws one item in a popup menu.
        if (menuItem->menuItemType == QStyleOptionMenuItem::Separator) {
            // Phantom ignores text and icons in menu separators, because
            // 1) The text and icons for separators don't render on Mac native menus
            // 2) There doesn't seem to be a way to account for the width of the text
            // properly (Fusion will often draw separator text clipped off)
            // 3) Setting text on separators also seems to mess up the metrics for
            // menu items on Mac native menus
            QRect r = option->rect;
            r.setHeight(r.height() / 2 + 1);
            Ph::fillRectEdges(painter, r, Qt::BottomEdge, Ph::Edge_Default_Thickness, 0, swatch,
                              S_window_divider);
            break;
        }
        const QRect itemRect = option->rect;
        painter->save();
        bool isSelected = menuItem->state & State_Selected && menuItem->state & State_Enabled;
        bool isCheckable = menuItem->checkType != QStyleOptionMenuItem::NotCheckable;
        bool isChecked = menuItem->checked;
        bool isSunken = menuItem->state & State_Sunken;
        bool isEnabled = menuItem->state & State_Enabled;
        bool hasSubMenu = menuItem->menuItemType == QStyleOptionMenuItem::SubMenu;
        if (isSelected) {
            Swatchy fillColor = isSunken ? S_highlight_outline : S_button_on;
            Ph::drawFilledRectNoBorder(painter, option->rect, Ph::Default_Rounding, swatch,
                                       fillColor);
        }

        if (isCheckable) {
            // Note: check rect might be misaligned vertically if it's a menu from a
            // combo box. Probably a bug in Qt code?
            QRect checkRect =
                    Ph::menuItemCheckRect(metrics, option->direction, itemRect, hasSubMenu);
            Swatchy signColor = !isEnabled ? S_windowText
                    : isSelected           ? S_highlightedText
                                           : S_windowText;
            if (menuItem->checkType & QStyleOptionMenuItem::Exclusive) {
                // Radio button
                if (isChecked) {
                    painter->setRenderHint(QPainter::Antialiasing);
                    painter->setPen(Qt::NoPen);
                    QPalette::ColorRole textRole = !isEnabled ? QPalette::Text
                            : isSelected                      ? QPalette::HighlightedText
                                                              : QPalette::ButtonText;
                    painter->setBrush(
                            option->palette.brush(option->palette.currentColorGroup(), textRole));
                    qreal rx, ry, rw, rh;
                    QRectF(checkRect).getRect(&rx, &ry, &rw, &rh);
                    qreal dim = (qreal)qMin(checkRect.width(), checkRect.height()) * 0.75;
                    QRectF rf(rx + rw / dim, ry + rh / dim, dim, dim);
                    painter->drawEllipse(rf);
                }
            } else {
                // If we want mouse-down to immediately show the item as
                // checked/unchecked (kinda bad if the user is click-holding on the
                // menu instead of click-clicking.)
                //
                // if ((isChecked && !isSunken) || (!isChecked && isSunken)) {
                if (isChecked) {
                    Ph::drawCheck(painter, d->checkBox_pen_scratch, checkRect, swatch, signColor);
                }
            }
        }

        const bool hasIcon = !menuItem->icon.isNull();

        if (hasIcon) {
            QRect iconRect = Ph::menuItemIconRect(metrics, option->direction, itemRect, hasSubMenu);
            QIcon::Mode mode = isEnabled ? QIcon::Normal : QIcon::Disabled;
            if (isSelected && isEnabled)
                mode = QIcon::Selected;
            QIcon::State state = isChecked ? QIcon::On : QIcon::Off;

            // TODO hmm, we might be ending up with blurry icons at size 15 instead
            // of 16 for example on Windows.
            //
            // int smallIconSize =
            //     proxy()->pixelMetric(PM_SmallIconSize, option, widget);
            // QSize iconSize(smallIconSize, smallIconSize);
            int iconExtent = qMin(iconRect.width(), iconRect.height());
            QSize iconSize(iconExtent, iconExtent);
#  if QT_CONFIG(combobox)
            if (auto combo = qobject_cast<const QComboBox *>(widget)) {
                iconSize = combo->iconSize();
            }
#  endif
            QWindow *window = widget ? widget->windowHandle() : nullptr;
#  if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
            QPixmap pixmap = menuItem->icon.pixmap(iconSize, 1.0, mode, state);
#  else
            QPixmap pixmap = menuItem->icon.pixmap(window, iconSize, mode, state);
#  endif
            const int pixw = (int)(pixmap.width() / pixmap.devicePixelRatio());
            const int pixh = (int)(pixmap.height() / pixmap.devicePixelRatio());
            QRect pixmapRect = QStyle::alignedRect(option->direction, Qt::AlignCenter,
                                                   QSize(pixw, pixh), iconRect);
            painter->drawPixmap(pixmapRect.topLeft(), pixmap);
        }

        // Draw main text and mnemonic text
        QStringView s(menuItem->text);
        if (!s.isEmpty()) {
#  if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
            QRect textRect = Ph::menuItemTextRect(metrics, option->direction, itemRect, hasSubMenu,
                                                  hasIcon, menuItem->reservedShortcutWidth);
#  else
            QRect textRect = Ph::menuItemTextRect(metrics, option->direction, itemRect, hasSubMenu,
                                                  hasIcon, menuItem->tabWidth);
#  endif
            int t = s.indexOf(QLatin1Char('\t'));
            int text_flags = Qt::AlignLeft | Qt::AlignTop | Qt::TextShowMnemonic | Qt::TextDontClip
                    | Qt::TextSingleLine;
            if (!styleHint(SH_UnderlineShortcut, menuItem, widget))
                text_flags |= Qt::TextHideMnemonic;
#  if 0
            painter->save();
#  endif
            painter->setPen(swatch.pen(isSelected ? S_highlightedText : S_text));

            // Comment from original Qt code which did some dance with the font:
            //
            // font may not have any "hard" flags set. We override the point size so
            // that when it is resolved against the device, this font will win. This
            // is mainly to handle cases where someone sets the font on the window
            // and then the combo inherits it and passes it onward. At that point the
            // resolve mask is very, very weak. This makes it stonger.
#  if 0
            QFont font = menuItem->font;
            font.setPointSizeF(QFontInfo(menuItem->font).pointSizeF());
            painter->setFont(font);
#  endif

            // My comment:
            //
            // What actually looks like is happening is that the qplatformtheme may
            // have set a per-class font for menus. The QComboMenuDelegate sets the
            // combo box's own font on the QStyleOptionMenuItem when passing it in
            // here and when calling sizeFromContents with CT_MenuItem, but the
            // QPainter we're called with hasn't had its font set to it -- it's still
            // set to the QMenu/QMenuItem app fonts hash font. So if it's a menu
            // coming from a combo box, let's just go ahead and set the font for it
            // if it doesn't match, since that's probably what it wanted to do. I
            // think. And as described above, we have to do the weird dance with the
            // resolve mask... which is some internal Qt detail that we aren't
            // supposed to have to deal with, but here we are.
            //
            // Ok, there's another problem, and QFusionStyle also suffers from it: in
            // high DPI, setting the pointSizeF and setting the font again won't
            // necessarily give us the right font (at least in Windows.) The font
            // might have too thin of a weight, and probably other problems. So just
            // forget about it: we'll have Phantom return 0 for the style hint that
            // the combo box uses to determine if it should use a QMenu popup instead
            // of a regular dropdown menu thing. The popup menu might actually be
            // better for usability in some cases, and it's how combos work on Mac
            // and BeOS, but it won't work anyway for editable combo boxes in Qt, and
            // the font issues just make it not worth it. So we'll have a dropdown
            // guy like a traditional Windows thing.
            //
            // If you want to try it out again, go to SH_ComboBox_Popup and have it
            // return 1.
            //
            // Alternatively, we could instead have the CT_MenuItem handling code try
            // to be aggressively clever and use the qt app font hash to look up the
            // expected font for a QMenu and use that for calculating its metrics.
            // Unfortunately, that probably won't work so great if the combo/menu
            // actually wants to use custom fonts in its listing, since we'd be
            // ignoring it. That's how UseQMenuForComboBoxPopup currently works,
            // though it tests for Qt::WA_SetFont as an attempt at recognizing when
            // it shouldn't use the qt font hash for QMenu.
#  if QT_CONFIG(combobox) && 0
            if (qobject_cast<const QComboBox *>(widget)) {
                QFont font = menuItem->font;
                font.setPointSizeF(QFontInfo(menuItem->font).pointSizeF());
                painter->setFont(font);
            }
#  endif

            // Draw mnemonic text
            if (t >= 0) {
#  if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
                QRect mnemonicR =
                        Ph::menuItemMnemonicRect(metrics, option->direction, itemRect, hasSubMenu,
                                                 menuItem->reservedShortcutWidth);
#  else
                QRect mnemonicR = Ph::menuItemMnemonicRect(metrics, option->direction, itemRect,
                                                           hasSubMenu, menuItem->tabWidth);
#  endif
                const QStringView textToDrawRef = s.mid(t + 1);
#  if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
                const QString unsafeTextToDraw =
                        QString::fromRawData(textToDrawRef.constData(), textToDrawRef.size());
#  else
                const QString unsafeTextToDraw =
                        QString::fromRawData(textToDrawRef.data(), textToDrawRef.size());
#  endif
                painter->drawText(mnemonicR, text_flags, unsafeTextToDraw);
                s = s.left(t);
            }
            const QStringView textToDrawRef = s.left(t);
#  if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
            const QString unsafeTextToDraw =
                    QString::fromRawData(textToDrawRef.constData(), textToDrawRef.size());
#  else
            const QString unsafeTextToDraw =
                    QString::fromRawData(textToDrawRef.data(), textToDrawRef.size());
#  endif
            painter->drawText(textRect, text_flags, unsafeTextToDraw);

#  if 0
            painter->restore();
#  endif
        }

        // SubMenu Arrow
        if (hasSubMenu) {
            int dim = (menuItem->rect.height() - 4) / 2;
            PrimitiveElement arrow;
            arrow = option->direction == Qt::RightToLeft ? PE_IndicatorArrowLeft
                                                         : PE_IndicatorArrowRight;
            int xpos = menuItem->rect.left() + menuItem->rect.width() - 3 - dim;
            QRect vSubMenuRect = visualRect(
                    option->direction, menuItem->rect,
                    QRect(xpos, menuItem->rect.top() + menuItem->rect.height() / 2 - dim / 2, dim,
                          dim));
            QStyleOptionMenuItem newMI = *menuItem;
            newMI.rect = vSubMenuRect;
            newMI.state = !isEnabled ? State_None : State_Enabled;
            if (isEnabled)
                newMI.palette.setColor(QPalette::WindowText,
                                       newMI.palette.highlightedText().color());
            proxy()->drawPrimitive(arrow, &newMI, painter, widget);
        }
        painter->restore();
        break;
    }
#endif // QT_CONFIG(menu)
    case CE_MenuHMargin:
    case CE_MenuVMargin:
    case CE_MenuEmptyArea:
        break;
    case CE_PushButton: {
        auto btn = qstyleoption_cast<const QStyleOptionButton *>(option);
        if (!btn)
            break;
        proxy()->drawControl(CE_PushButtonBevel, btn, painter, widget);
        QStyleOptionButton subopt = *btn;
        subopt.rect = subElementRect(SE_PushButtonContents, btn, widget);
        proxy()->drawControl(CE_PushButtonLabel, &subopt, painter, widget);
        break;
    }
    case CE_PushButtonLabel: {
        auto button = qstyleoption_cast<const QStyleOptionButton *>(option);
        if (!button)
            break;
        // This code is very similar to QCommonStyle's implementation, but doesn't
        // set the icon mode to active when focused.
        QRect textRect = button->rect;
        int tf = Qt::AlignVCenter | Qt::TextShowMnemonic;
        if (!proxy()->styleHint(SH_UnderlineShortcut, button, widget))
            tf |= Qt::TextHideMnemonic;
        if (!button->icon.isNull()) {
            // Center both icon and text
            QRect iconRect;
            QIcon::Mode mode = button->state & State_Enabled ? QIcon::Normal : QIcon::Disabled;
            QIcon::State state = button->state & State_On ? QIcon::On : QIcon::Off;
            auto window = widget ? widget->window()->windowHandle() : nullptr;
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
            QPixmap pixmap = button->icon.pixmap(button->iconSize, 1.0, mode, state);
#else
            QPixmap pixmap = button->icon.pixmap(window, button->iconSize, mode, state);
#endif
            int pixmapWidth = (int)((qreal)pixmap.width() / pixmap.devicePixelRatio());
            int pixmapHeight = (int)((qreal)pixmap.height() / pixmap.devicePixelRatio());
            int labelWidth = pixmapWidth;
            int labelHeight = pixmapHeight;
            // 4 is hardcoded in QPushButton::sizeHint()
            int iconSpacing = 4;
            int textWidth =
                    button->fontMetrics.boundingRect(option->rect, tf, button->text).width();
            if (!button->text.isEmpty())
                labelWidth += (textWidth + iconSpacing);
            iconRect = QRect(textRect.x() + (textRect.width() - labelWidth) / 2,
                             textRect.y() + (textRect.height() - labelHeight) / 2, pixmapWidth,
                             pixmapHeight);
            iconRect = visualRect(button->direction, textRect, iconRect);
            tf |= Qt::AlignLeft; // left align, we adjust the text-rect instead
            if (button->direction == Qt::RightToLeft)
                textRect.setRight(iconRect.left() - iconSpacing);
            else
                textRect.setLeft(iconRect.left() + iconRect.width() + iconSpacing);
            if (button->state & (State_On | State_Sunken))
                iconRect.translate(proxy()->pixelMetric(PM_ButtonShiftHorizontal, option, widget),
                                   proxy()->pixelMetric(PM_ButtonShiftVertical, option, widget));
            painter->drawPixmap(iconRect, pixmap);
        } else {
            tf |= Qt::AlignHCenter;
        }
        if (button->state & (State_On | State_Sunken))
            textRect.translate(proxy()->pixelMetric(PM_ButtonShiftHorizontal, option, widget),
                               proxy()->pixelMetric(PM_ButtonShiftVertical, option, widget));
        if (button->features & QStyleOptionButton::HasMenu) {
            int indicatorSize = proxy()->pixelMetric(PM_MenuButtonIndicator, button, widget);
            if (button->direction == Qt::LeftToRight)
                textRect = textRect.adjusted(0, 0, -indicatorSize, 0);
            else
                textRect = textRect.adjusted(indicatorSize, 0, 0, 0);
        }
        proxy()->drawItemText(painter, textRect, tf, button->palette,
                              (button->state & State_Enabled), button->text, QPalette::ButtonText);
        break;
    }
    case CE_MenuBarEmptyArea: {
        if (QWin11Phantom::hasTweakTrue(widget, QWin11Phantom::Tweak::menubar_no_ruler))
            break;
        QRect rect = option->rect;
        painter->fillRect(rect, swatch.color(S_window));
        Ph::fillRectEdges(painter, rect, Qt::BottomEdge, Ph::Edge_Default_Thickness, 0, swatch,
                          S_window_divider);
        break;
    }
#if QT_CONFIG(tabbar)
    case CE_TabBarTabShape: {
        auto tab = qstyleoption_cast<const QStyleOptionTab *>(option);
        if (!tab)
            break;
        bool rtlHorTabs =
                (tab->direction == Qt::RightToLeft
                 && (tab->shape == QTabBar::RoundedNorth || tab->shape == QTabBar::RoundedSouth));
        bool isSelected = tab->state & State_Selected;
        bool lastTab = ((!rtlHorTabs && tab->position == QStyleOptionTab::End)
                        || (rtlHorTabs && tab->position == QStyleOptionTab::Beginning));
        bool onlyOne = tab->position == QStyleOptionTab::OnlyOneTab;
        int tabOverlap = pixelMetric(PM_TabBarTabOverlap, option, widget);
        const qreal rounding = Ph::TabBarTab_Rounding;
        Qt::Edge outerEdge = Qt::TopEdge;
        Qt::Edge edgeTowardNextTab = Qt::RightEdge;
        switch (tab->shape) {
        case QTabBar::RoundedNorth:
            outerEdge = Qt::TopEdge;
            edgeTowardNextTab = Qt::RightEdge;
            break;
        case QTabBar::RoundedSouth:
            outerEdge = Qt::BottomEdge;
            edgeTowardNextTab = Qt::RightEdge;
            break;
        case QTabBar::RoundedWest:
            outerEdge = Qt::LeftEdge;
            edgeTowardNextTab = Qt::BottomEdge;
            break;
        case QTabBar::RoundedEast:
            outerEdge = Qt::RightEdge;
            edgeTowardNextTab = Qt::BottomEdge;
            break;
        default:
            QCommonStyle::drawControl(element, tab, painter, widget);
            return;
        }
        Qt::Edge innerEdge = Ph::oppositeEdge(outerEdge);
        Qt::Edge edgeAwayNextTab = Ph::oppositeEdge(edgeTowardNextTab);
        QRectF shapeClipRect = Ph::expandRect(option->rect, innerEdge, -2);
        QRectF drawRect = Ph::expandRect(shapeClipRect, innerEdge, 3 + 2 * (int)rounding + 1);
        if (!onlyOne && !lastTab) {
            drawRect = Ph::expandRect(drawRect, edgeTowardNextTab, tabOverlap);
            shapeClipRect = Ph::expandRect(shapeClipRect, edgeTowardNextTab, tabOverlap);
        }
        if (!isSelected) {
            int offset = proxy()->pixelMetric(PM_TabBarTabShiftVertical, option, widget);
            drawRect = Ph::expandRect(drawRect, outerEdge, -offset);
        }
        painter->save();
        painter->setClipRect(shapeClipRect);
        bool hasFrame = tab->features & QStyleOptionTab::HasFrame && !tab->documentMode;
        Swatchy tabFrameColor, thisFillColor, specular;
        if (hasFrame) {
            tabFrameColor = S_tabFrame;
            if (isSelected) {
                thisFillColor = S_tabFrame;
                specular = S_tabFrame_specular;
            } else {
                thisFillColor = S_inactiveTabYesFrame;
                specular = Ph::TabBar_InactiveTabsHaveSpecular ? S_inactiveTabYesFrame_specular
                                                               : S_none;
            }
        } else {
            tabFrameColor = S_window;
            if (isSelected) {
                thisFillColor = S_window;
                specular = S_window_specular;
            } else {
                thisFillColor = S_inactiveTabNoFrame;
                specular = Ph::TabBar_InactiveTabsHaveSpecular ? S_inactiveTabNoFrame_specular
                                                               : S_none;
            }
        }
        Ph::drawFilledRect(painter, drawRect, rounding, Ph::Default_Thickness, swatch,
                           thisFillColor, S_window_outline);
        Ph::drawRoundedBorder(painter,
                              drawRect.adjusted(Ph::Default_Thickness, Ph::Default_Thickness,
                                                -Ph::Default_Thickness, -Ph::Default_Thickness),
                              Ph::Default_Thickness, rounding, swatch, specular);
        painter->restore();
        if (isSelected) {
            painter->save();
            QRectF refillRect = Ph::rectFromInnerEdgeWithThickness(shapeClipRect, innerEdge, 2);
            refillRect = Ph::rectTranslatedTowardEdge(refillRect, innerEdge, 2);
            refillRect = Ph::expandRect(refillRect, edgeAwayNextTab | edgeTowardNextTab,
                                        -Ph::Default_Thickness);
            Ph::drawFilledRectNoBorder(painter, refillRect, Ph::Default_Rounding, swatch,
                                       tabFrameColor);
            Ph::fillRectEdges(painter, refillRect, edgeAwayNextTab | edgeTowardNextTab,
                              Ph::Edge_Default_Thickness, Ph::Default_Rounding, swatch, specular);
            painter->restore();
        }
        break;
    }
#endif // QT_CONFIG(tabbar)
#if QT_CONFIG(itemviews)
    case CE_ItemViewItem: {
        auto ivopt = qstyleoption_cast<const QStyleOptionViewItem *>(option);
        if (!ivopt)
            break;
        // Hack to work around broken grid line drawing in Qt's table view code:
        //
        // We tell it that the grid line color is a color via
        // SH_Table_GridLineColor. It draws the grid lines, but it in high DPI it's
        // broken because it uses a pen/path to draw the line, which makes it too
        // narrow, subpixel-incorrectly-antialiased, and/or offset from its correct
        // position. So when we draw the item view items in a table view, we'll
        // also try to paint 1 pixel outside of our current rect to try to fill in
        // the incorrectly painted areas where the grid lines are.
        //
        // Also note that the table views with the bad drawing code, when
        // scrolling, will leave garbage behind in the incorrectly-drawn grid line
        // areas. This will also paint over that.
        bool overdrawGridHack = false;
        if (auto tableWidget = qobject_cast<const QTableView *>(widget)) {
            overdrawGridHack = tableWidget->showGrid() && tableWidget->gridStyle() == Qt::SolidLine;
        }
        if (overdrawGridHack) {
            auto r = QRectF(option->rect)
                             .adjusted(-Ph::Default_Thickness, -Ph::Default_Thickness,
                                       Ph::Default_Thickness, Ph::Default_Thickness);
            Ph::drawRoundedBorder(painter, r, QWin11Phantom::Default_Thickness, 0, swatch,
                                  S_base_divider);
        }

        QCommonStyle::drawControl(element, option, painter, widget);
        break;
    }
#endif
    case CE_ShapedFrame: {
        auto frameopt = qstyleoption_cast<const QStyleOptionFrame *>(option);
        if (frameopt) {
            if (frameopt->frameShape == QFrame::HLine) {
                QRectF r = option->rect;
                r.setY(r.y() + r.height() / 2);
                r.setHeight(Ph::Default_Thickness);
                painter->fillRect(r, swatch.color(S_window_outline));
                break;
            } else if (frameopt->frameShape == QFrame::VLine) {
                QRectF r = option->rect;
                r.setX(r.x() + r.width() / 2);
                r.setWidth(Ph::Default_Thickness);
                painter->fillRect(r, swatch.color(S_window_outline));
                break;
            }
        }
        QCommonStyle::drawControl(element, option, painter, widget);
        break;
    }
    case CE_PushButtonBevel: /* Override this so we also paint if mouse over on flat buttons */
        if (const QStyleOptionButton *btn = qstyleoption_cast<const QStyleOptionButton *>(option)) {
            QRect br = btn->rect;
            int dbi = proxy()->pixelMetric(PM_ButtonDefaultIndicator, btn, widget);
            if (btn->features & QStyleOptionButton::DefaultButton)
                proxy()->drawPrimitive(PE_FrameDefaultButton, option, painter, widget);
            if (btn->features & QStyleOptionButton::AutoDefaultButton)
                br.setCoords(br.left() + dbi, br.top() + dbi, br.right() - dbi, br.bottom() - dbi);
            if (!(btn->features
                  & (QStyleOptionButton::Flat | QStyleOptionButton::CommandLinkButton))
                || btn->state & (State_Sunken | State_On | State_MouseOver)) {
                QStyleOptionButton tmpBtn = *btn;
                tmpBtn.rect = br;
                proxy()->drawPrimitive(PE_PanelButtonCommand, &tmpBtn, painter, widget);
            }
            if (btn->features & QStyleOptionButton::HasMenu) {
                int mbi = proxy()->pixelMetric(PM_MenuButtonIndicator, btn, widget);
                QRect ir = btn->rect;
                QStyleOptionButton newBtn = *btn;
                newBtn.rect = QRect(ir.right() - mbi + 2, ir.height() / 2 - mbi / 2 + 3, mbi - 6,
                                    mbi - 6);
                newBtn.rect = visualRect(btn->direction, br, newBtn.rect);
                proxy()->drawPrimitive(PE_IndicatorArrowDown, &newBtn, painter, widget);
            }
        }
        break;
    default:
        QCommonStyle::drawControl(element, option, painter, widget);
        break;
    }
}

QPalette QWin11PhantomStyle::standardPalette() const
{
    return QCommonStyle::standardPalette();
}

void QWin11PhantomStyle::drawComplexControl(ComplexControl control,
                                            const QStyleOptionComplex *option, QPainter *painter,
                                            const QWidget *widget) const
{
#ifdef BUILD_WITH_EASY_PROFILER
    EASY_BLOCK("drawControl");
    const char *controlCString = QMetaEnum::fromType<QStyle::ComplexControl>().valueToKey(control);
    EASY_TEXT("ComplexControl", controlCString);
#endif
    using Swatchy = QWin11Phantom::Swatchy;
    using namespace QWin11Phantom::SwatchColors;
    namespace Ph = QWin11Phantom;
    auto ph_swatchPtr =
            Ph::getCachedSwatchOfQPalette(&d->swatchCache, &d->headSwatchFastKey, option->palette);
    const Ph::PhSwatch &swatch = *ph_swatchPtr.data();

    switch (control) {
    case CC_GroupBox: {
        auto groupBox = qstyleoption_cast<const QStyleOptionGroupBox *>(option);
        if (!groupBox)
            break;
        painter->save();
        // Draw frame
        QRect textRect = proxy()->subControlRect(CC_GroupBox, option, SC_GroupBoxLabel, widget);
        QRect checkBoxRect =
                proxy()->subControlRect(CC_GroupBox, option, SC_GroupBoxCheckBox, widget);

        if (groupBox->subControls & QStyle::SC_GroupBoxFrame) {
            QStyleOptionFrame frame;
            frame.QStyleOption::operator=(*groupBox);
            frame.features = groupBox->features;
            frame.lineWidth = groupBox->lineWidth;
            frame.midLineWidth = groupBox->midLineWidth;
            frame.rect = proxy()->subControlRect(CC_GroupBox, option, SC_GroupBoxFrame, widget);
            proxy()->drawPrimitive(PE_FrameGroupBox, &frame, painter, widget);
        }

        // Draw title
        if ((groupBox->subControls & QStyle::SC_GroupBoxLabel) && !groupBox->text.isEmpty()) {
            // groupBox->textColor gets the incorrect palette here
            painter->setPen(QPen(option->palette.windowText(), 1));
            int alignment = int(groupBox->textAlignment);
            if (!proxy()->styleHint(QStyle::SH_UnderlineShortcut, option, widget))
                alignment |= Qt::TextHideMnemonic;

            proxy()->drawItemText(painter, textRect,
                                  // int cast to suppress clang analyzer warning
                                  (int)Qt::TextShowMnemonic | (int)Qt::AlignLeft | alignment,
                                  groupBox->palette, groupBox->state & State_Enabled,
                                  groupBox->text, QPalette::NoRole);

            if (groupBox->state & State_HasFocus) {
                QStyleOptionFocusRect fropt;
                fropt.QStyleOption::operator=(*groupBox);
                fropt.rect = textRect.adjusted(-1, 0, 1, 0);
                proxy()->drawPrimitive(PE_FrameFocusRect, &fropt, painter, widget);
            }
        }

        // Draw checkbox
        if (groupBox->subControls & SC_GroupBoxCheckBox) {
            QStyleOptionButton box;
            box.QStyleOption::operator=(*groupBox);
            box.rect = checkBoxRect;
            proxy()->drawPrimitive(PE_IndicatorCheckBox, &box, painter, widget);
        }
        painter->restore();
        break;
    }
#if QT_CONFIG(spinbox)
    case CC_SpinBox: {
        auto spinBox = qstyleoption_cast<const QStyleOptionSpinBox *>(option);
        if (!spinBox)
            break;
        bool isLeftToRight = option->direction != Qt::RightToLeft;
        const QRectF rect = spinBox->rect;
        bool sunken = spinBox->state & State_Sunken;
        QRect upRect = proxy()->subControlRect(CC_SpinBox, spinBox, SC_SpinBoxUp, widget);
        QRect downRect = proxy()->subControlRect(CC_SpinBox, spinBox, SC_SpinBoxDown, widget);

        Swatchy buttonFill = sunken ? S_button_pressed : S_button;

        Ph::drawFilledRectNoBorder(painter, rect, Ph::Default_Rounding, swatch, buttonFill);

        if (spinBox->frame) {
            QStyleOptionFrame frameOption;
            frameOption.QStyleOption::operator=(*spinBox);
            frameOption.rect = spinBox->rect;
            frameOption.state =
                    (spinBox->state & (State_Enabled | State_MouseOver | State_HasFocus))
                    | State_KeyboardFocusChange;
            if (sunken) {
                frameOption.state |= State_Sunken;
                frameOption.state &= ~State_MouseOver;
            }

            proxy()->drawPrimitive(PE_FrameLineEdit, &frameOption, painter, widget);
        }

        if (spinBox->buttonSymbols == QAbstractSpinBox::PlusMinus) {
            Ph::PSave save(painter);
            // TODO fix up old fusion code here
            int centerX = upRect.center().x();
            int centerY = upRect.center().y();
            Swatchy arrowColorUp = spinBox->stepEnabled & QAbstractSpinBox::StepUpEnabled
                    ? S_indicator_current
                    : S_indicator_disabled;
            Swatchy arrowColorDown = spinBox->stepEnabled & QAbstractSpinBox::StepDownEnabled
                    ? S_indicator_current
                    : S_indicator_disabled;
            painter->setPen(swatch.pen(arrowColorUp));
            painter->drawLine(centerX - 1, centerY, centerX + 3, centerY);
            painter->drawLine(centerX + 1, centerY - 2, centerX + 1, centerY + 2);
            centerX = downRect.center().x();
            centerY = downRect.center().y();
            painter->setPen(arrowColorDown);
            painter->drawLine(centerX - 1, centerY, centerX + 3, centerY);
        } else if (spinBox->buttonSymbols == QAbstractSpinBox::UpDownArrows) {
            int xoffs = isLeftToRight ? 0 : 1;
            Ph::drawArrow(painter, upRect.adjusted(4 + xoffs, 1, -5 + xoffs, 1), Qt::UpArrow,
                          swatch, spinBox->stepEnabled & QAbstractSpinBox::StepUpEnabled);
            Ph::drawArrow(painter, downRect.adjusted(4 + xoffs, 0, -5 + xoffs, -1), Qt::DownArrow,
                          swatch, spinBox->stepEnabled & QAbstractSpinBox::StepDownEnabled);
        }
        break;
    }
#endif // QT_CONFIG(spinbox)
    case CC_TitleBar: {
        auto titleBar = qstyleoption_cast<const QStyleOptionTitleBar *>(option);
        if (!titleBar)
            break;
        painter->save();
        const int buttonMargin = 5;
        bool active = (titleBar->titleBarState & State_Active);
        QRect fullRect = titleBar->rect;
        QPalette palette = option->palette;
        QColor highlight = option->palette.highlight().color();
        QColor outline = option->palette.dark().color();

        QColor titleBarFrameBorder(active ? highlight.darker(180) : outline.darker(110));
        QColor titleBarHighlight(active ? highlight.lighter(120)
                                        : palette.window().color().lighter(120));
        QColor textColor(active ? 0xffffff : 0xff000000);
        QColor textAlphaColor(active ? 0xffffff : 0xff000000);

        {
            // Fill title
            QColor titlebarColor = QColor(active ? highlight : palette.window().color());
            painter->fillRect(option->rect.adjusted(1, 1, -1, 0), titlebarColor);
            // Frame and rounded corners
            painter->setPen(titleBarFrameBorder);

            // top outline
            painter->drawLine(fullRect.left() + 5, fullRect.top(), fullRect.right() - 5,
                              fullRect.top());
            painter->drawLine(fullRect.left(), fullRect.top() + 4, fullRect.left(),
                              fullRect.bottom());
            const QPoint points[5] = { QPoint(fullRect.left() + 4, fullRect.top() + 1),
                                       QPoint(fullRect.left() + 3, fullRect.top() + 1),
                                       QPoint(fullRect.left() + 2, fullRect.top() + 2),
                                       QPoint(fullRect.left() + 1, fullRect.top() + 3),
                                       QPoint(fullRect.left() + 1, fullRect.top() + 4) };
            painter->drawPoints(points, 5);

            painter->drawLine(fullRect.right(), fullRect.top() + 4, fullRect.right(),
                              fullRect.bottom());
            const QPoint points2[5] = { QPoint(fullRect.right() - 3, fullRect.top() + 1),
                                        QPoint(fullRect.right() - 4, fullRect.top() + 1),
                                        QPoint(fullRect.right() - 2, fullRect.top() + 2),
                                        QPoint(fullRect.right() - 1, fullRect.top() + 3),
                                        QPoint(fullRect.right() - 1, fullRect.top() + 4) };
            painter->drawPoints(points2, 5);

            // draw bottomline
            painter->drawLine(fullRect.right(), fullRect.bottom(), fullRect.left(),
                              fullRect.bottom());

            // top highlight
            painter->setPen(titleBarHighlight);
            painter->drawLine(fullRect.left() + 6, fullRect.top() + 1, fullRect.right() - 6,
                              fullRect.top() + 1);
        }
        // draw title
        QRect textRect = proxy()->subControlRect(CC_TitleBar, titleBar, SC_TitleBarLabel, widget);
        painter->setPen(active ? (titleBar->palette.text().color().lighter(120))
                               : titleBar->palette.text().color());
        // Note workspace also does elliding but it does not use the correct font
        QString title = painter->fontMetrics().elidedText(titleBar->text, Qt::ElideRight,
                                                          textRect.width() - 14);
        painter->drawText(textRect.adjusted(1, 1, 1, 1), title,
                          QTextOption(Qt::AlignHCenter | Qt::AlignVCenter));
        painter->setPen(Qt::white);
        if (active)
            painter->drawText(textRect, title, QTextOption(Qt::AlignHCenter | Qt::AlignVCenter));
        // min button
        if ((titleBar->subControls & SC_TitleBarMinButton)
            && (titleBar->titleBarFlags & Qt::WindowMinimizeButtonHint)
            && !(titleBar->titleBarState & Qt::WindowMinimized)) {
            QRect minButtonRect =
                    proxy()->subControlRect(CC_TitleBar, titleBar, SC_TitleBarMinButton, widget);
            if (minButtonRect.isValid()) {
                bool hover = (titleBar->activeSubControls & SC_TitleBarMinButton)
                        && (titleBar->state & State_MouseOver);
                bool sunken = (titleBar->activeSubControls & SC_TitleBarMinButton)
                        && (titleBar->state & State_Sunken);
                Ph::drawMdiButton(painter, titleBar, minButtonRect, hover, sunken);
                QRect minButtonIconRect = minButtonRect.adjusted(buttonMargin, buttonMargin,
                                                                 -buttonMargin, -buttonMargin);
                painter->setPen(textColor);
                painter->drawLine(
                        minButtonIconRect.center().x() - 2, minButtonIconRect.center().y() + 3,
                        minButtonIconRect.center().x() + 3, minButtonIconRect.center().y() + 3);
                painter->drawLine(
                        minButtonIconRect.center().x() - 2, minButtonIconRect.center().y() + 4,
                        minButtonIconRect.center().x() + 3, minButtonIconRect.center().y() + 4);
                painter->setPen(textAlphaColor);
                painter->drawLine(
                        minButtonIconRect.center().x() - 3, minButtonIconRect.center().y() + 3,
                        minButtonIconRect.center().x() - 3, minButtonIconRect.center().y() + 4);
                painter->drawLine(
                        minButtonIconRect.center().x() + 4, minButtonIconRect.center().y() + 3,
                        minButtonIconRect.center().x() + 4, minButtonIconRect.center().y() + 4);
            }
        }
        // max button
        if ((titleBar->subControls & SC_TitleBarMaxButton)
            && (titleBar->titleBarFlags & Qt::WindowMaximizeButtonHint)
            && !(titleBar->titleBarState & Qt::WindowMaximized)) {
            QRect maxButtonRect =
                    proxy()->subControlRect(CC_TitleBar, titleBar, SC_TitleBarMaxButton, widget);
            if (maxButtonRect.isValid()) {
                bool hover = (titleBar->activeSubControls & SC_TitleBarMaxButton)
                        && (titleBar->state & State_MouseOver);
                bool sunken = (titleBar->activeSubControls & SC_TitleBarMaxButton)
                        && (titleBar->state & State_Sunken);
                Ph::drawMdiButton(painter, titleBar, maxButtonRect, hover, sunken);

                QRect maxButtonIconRect = maxButtonRect.adjusted(buttonMargin, buttonMargin,
                                                                 -buttonMargin, -buttonMargin);

                painter->setPen(textColor);
                painter->drawRect(maxButtonIconRect.adjusted(0, 0, -1, -1));
                painter->drawLine(maxButtonIconRect.left() + 1, maxButtonIconRect.top() + 1,
                                  maxButtonIconRect.right() - 1, maxButtonIconRect.top() + 1);
                painter->setPen(textAlphaColor);
                const QPoint points[4] = { maxButtonIconRect.topLeft(),
                                           maxButtonIconRect.topRight(),
                                           maxButtonIconRect.bottomLeft(),
                                           maxButtonIconRect.bottomRight() };
                painter->drawPoints(points, 4);
            }
        }

        // close button
        if ((titleBar->subControls & SC_TitleBarCloseButton)
            && (titleBar->titleBarFlags & Qt::WindowSystemMenuHint)) {
            QRect closeButtonRect =
                    proxy()->subControlRect(CC_TitleBar, titleBar, SC_TitleBarCloseButton, widget);
            if (closeButtonRect.isValid()) {
                bool hover = (titleBar->activeSubControls & SC_TitleBarCloseButton)
                        && (titleBar->state & State_MouseOver);
                bool sunken = (titleBar->activeSubControls & SC_TitleBarCloseButton)
                        && (titleBar->state & State_Sunken);
                Ph::drawMdiButton(painter, titleBar, closeButtonRect, hover, sunken);
                QRect closeIconRect = closeButtonRect.adjusted(buttonMargin, buttonMargin,
                                                               -buttonMargin, -buttonMargin);
                painter->setPen(textAlphaColor);
                const QLine lines[4] = { QLine(closeIconRect.left() + 1, closeIconRect.top(),
                                               closeIconRect.right(), closeIconRect.bottom() - 1),
                                         QLine(closeIconRect.left(), closeIconRect.top() + 1,
                                               closeIconRect.right() - 1, closeIconRect.bottom()),
                                         QLine(closeIconRect.right() - 1, closeIconRect.top(),
                                               closeIconRect.left(), closeIconRect.bottom() - 1),
                                         QLine(closeIconRect.right(), closeIconRect.top() + 1,
                                               closeIconRect.left() + 1, closeIconRect.bottom()) };
                painter->drawLines(lines, 4);
                const QPoint points[4] = { closeIconRect.topLeft(), closeIconRect.topRight(),
                                           closeIconRect.bottomLeft(),
                                           closeIconRect.bottomRight() };
                painter->drawPoints(points, 4);

                painter->setPen(textColor);
                painter->drawLine(closeIconRect.left() + 1, closeIconRect.top() + 1,
                                  closeIconRect.right() - 1, closeIconRect.bottom() - 1);
                painter->drawLine(closeIconRect.left() + 1, closeIconRect.bottom() - 1,
                                  closeIconRect.right() - 1, closeIconRect.top() + 1);
            }
        }

        // normalize button
        if ((titleBar->subControls & SC_TitleBarNormalButton)
            && (((titleBar->titleBarFlags & Qt::WindowMinimizeButtonHint)
                 && (titleBar->titleBarState & Qt::WindowMinimized))
                || ((titleBar->titleBarFlags & Qt::WindowMaximizeButtonHint)
                    && (titleBar->titleBarState & Qt::WindowMaximized)))) {
            QRect normalButtonRect =
                    proxy()->subControlRect(CC_TitleBar, titleBar, SC_TitleBarNormalButton, widget);
            if (normalButtonRect.isValid()) {

                bool hover = (titleBar->activeSubControls & SC_TitleBarNormalButton)
                        && (titleBar->state & State_MouseOver);
                bool sunken = (titleBar->activeSubControls & SC_TitleBarNormalButton)
                        && (titleBar->state & State_Sunken);
                QRect normalButtonIconRect = normalButtonRect.adjusted(
                        buttonMargin, buttonMargin, -buttonMargin, -buttonMargin);
                Ph::drawMdiButton(painter, titleBar, normalButtonRect, hover, sunken);

                QRect frontWindowRect = normalButtonIconRect.adjusted(0, 3, -3, 0);
                painter->setPen(textColor);
                painter->drawRect(frontWindowRect.adjusted(0, 0, -1, -1));
                painter->drawLine(frontWindowRect.left() + 1, frontWindowRect.top() + 1,
                                  frontWindowRect.right() - 1, frontWindowRect.top() + 1);
                painter->setPen(textAlphaColor);
                const QPoint points[4] = { frontWindowRect.topLeft(), frontWindowRect.topRight(),
                                           frontWindowRect.bottomLeft(),
                                           frontWindowRect.bottomRight() };
                painter->drawPoints(points, 4);

                QRect backWindowRect = normalButtonIconRect.adjusted(3, 0, 0, -3);
                QRegion clipRegion = backWindowRect;
                clipRegion -= frontWindowRect;
                painter->save();
                painter->setClipRegion(clipRegion);
                painter->setPen(textColor);
                painter->drawRect(backWindowRect.adjusted(0, 0, -1, -1));
                painter->drawLine(backWindowRect.left() + 1, backWindowRect.top() + 1,
                                  backWindowRect.right() - 1, backWindowRect.top() + 1);
                painter->setPen(textAlphaColor);
                const QPoint points2[4] = { backWindowRect.topLeft(), backWindowRect.topRight(),
                                            backWindowRect.bottomLeft(),
                                            backWindowRect.bottomRight() };
                painter->drawPoints(points2, 4);
                painter->restore();
            }
        }

        // context help button
        if (titleBar->subControls & SC_TitleBarContextHelpButton
            && (titleBar->titleBarFlags & Qt::WindowContextHelpButtonHint)) {
            QRect contextHelpButtonRect = proxy()->subControlRect(
                    CC_TitleBar, titleBar, SC_TitleBarContextHelpButton, widget);
            if (contextHelpButtonRect.isValid()) {
                bool hover = (titleBar->activeSubControls & SC_TitleBarContextHelpButton)
                        && (titleBar->state & State_MouseOver);
                bool sunken = (titleBar->activeSubControls & SC_TitleBarContextHelpButton)
                        && (titleBar->state & State_Sunken);
                Ph::drawMdiButton(painter, titleBar, contextHelpButtonRect, hover, sunken);
                // This is lame, but I doubt it will get used often. Previously, XPM
                // icon was used here (very poorly, by re-allocating a QImage over and
                // over and modifying/painting it)
                QIcon helpIcon = QCommonStyle::standardIcon(QStyle::SP_DialogHelpButton);
                helpIcon.paint(painter, contextHelpButtonRect.adjusted(4, 4, -4, -4));
            }
        }

        // shade button
        if (titleBar->subControls & SC_TitleBarShadeButton
            && (titleBar->titleBarFlags & Qt::WindowShadeButtonHint)) {
            QRect shadeButtonRect =
                    proxy()->subControlRect(CC_TitleBar, titleBar, SC_TitleBarShadeButton, widget);
            if (shadeButtonRect.isValid()) {
                bool hover = (titleBar->activeSubControls & SC_TitleBarShadeButton)
                        && (titleBar->state & State_MouseOver);
                bool sunken = (titleBar->activeSubControls & SC_TitleBarShadeButton)
                        && (titleBar->state & State_Sunken);
                Ph::drawMdiButton(painter, titleBar, shadeButtonRect, hover, sunken);
                Ph::drawArrow(painter, shadeButtonRect.adjusted(5, 7, -5, -7), Qt::UpArrow, swatch);
            }
        }

        // unshade button
        if (titleBar->subControls & SC_TitleBarUnshadeButton
            && (titleBar->titleBarFlags & Qt::WindowShadeButtonHint)) {
            QRect unshadeButtonRect = proxy()->subControlRect(CC_TitleBar, titleBar,
                                                              SC_TitleBarUnshadeButton, widget);
            if (unshadeButtonRect.isValid()) {
                bool hover = (titleBar->activeSubControls & SC_TitleBarUnshadeButton)
                        && (titleBar->state & State_MouseOver);
                bool sunken = (titleBar->activeSubControls & SC_TitleBarUnshadeButton)
                        && (titleBar->state & State_Sunken);
                Ph::drawMdiButton(painter, titleBar, unshadeButtonRect, hover, sunken);
                Ph::drawArrow(painter, unshadeButtonRect.adjusted(5, 7, -5, -7), Qt::DownArrow,
                              swatch);
            }
        }

        if ((titleBar->subControls & SC_TitleBarSysMenu)
            && (titleBar->titleBarFlags & Qt::WindowSystemMenuHint)) {
            QRect iconRect =
                    proxy()->subControlRect(CC_TitleBar, titleBar, SC_TitleBarSysMenu, widget);
            if (iconRect.isValid()) {
                if (!titleBar->icon.isNull()) {
                    titleBar->icon.paint(painter, iconRect);
                } else {
                    QStyleOption tool = *titleBar;
                    QPixmap pm = proxy()->standardIcon(SP_TitleBarMenuButton, &tool, widget)
                                         .pixmap(16, 16);
                    tool.rect = iconRect;
                    painter->save();
                    proxy()->drawItemPixmap(painter, iconRect, Qt::AlignCenter, pm);
                    painter->restore();
                }
            }
        }
        painter->restore();
        break;
    }
#if QT_CONFIG(slider)
    case CC_ScrollBar: {
        auto scrollBar = qstyleoption_cast<const QStyleOptionSlider *>(option);
        if (!scrollBar)
            break;

        bool isHorizontal = scrollBar->orientation == Qt::Horizontal;

        /* Have to do this because scrollbar has the Qt::WA_OpaquePaintEvent widget attribute set */
        //        painter->fillRect(scrollBar->rect, swatch.color(S_window));
        bool isMouseOver = scrollBar->state & State_MouseOver;

        auto rect = scrollBar->rect;
        if (!isMouseOver) {
            if (isHorizontal) {
                rect.setTop(rect.bottom() - Ph::SliderDefaultWidth);
            } else {
                rect.setLeft(rect.right() - Ph::SliderDefaultWidth);
            }
        }

        Ph::drawFilledRectNoBorder(painter, rect, Ph::Default_Rounding, swatch, S_window);

        bool isLeftToRight = option->direction != Qt::RightToLeft;
        auto pr = proxy();

        bool isEnabled = scrollBar->state & State_Enabled;
        bool hasRange = scrollBar->minimum != scrollBar->maximum;

        // Slider thumb
        if (scrollBar->subControls & SC_ScrollBarSlider && hasRange) {
            QRect scrollBarSlider =
                    pr->subControlRect(control, scrollBar, SC_ScrollBarSlider, widget);

            if (!isMouseOver) {
                if (isHorizontal) {
                    scrollBarSlider.setTop(scrollBarSlider.bottom() - Ph::SliderDefaultWidth);
                } else {
                    scrollBarSlider.setLeft(scrollBarSlider.right() - Ph::SliderDefaultWidth);
                }
            }

            Ph::drawFilledRectNoBorder(painter, scrollBarSlider,
                                       isMouseOver ? Ph::SliderHandle_Rounding : 1, swatch,
                                       isEnabled ? S_indicator_current : S_indicator_disabled);
        }

        // The SubLine (up/left) buttons
        if (isMouseOver && scrollBar->subControls & SC_ScrollBarSubLine) {
            QRect scrollBarSubLine =
                    pr->subControlRect(control, scrollBar, SC_ScrollBarSubLine, widget);
            if (!isMouseOver) {
                if (isHorizontal) {
                    scrollBarSubLine.setTop(scrollBarSubLine.bottom() - Ph::SliderDefaultWidth);
                } else {
                    scrollBarSubLine.setLeft(scrollBarSubLine.right() - Ph::SliderDefaultWidth);
                }
            }

            // Arrows
            Qt::ArrowType arrowType;
            if (isHorizontal) {
                arrowType = isLeftToRight ? Qt::LeftArrow : Qt::RightArrow;
            } else {
                arrowType = Qt::UpArrow;
            }
            Ph::drawArrow(painter, scrollBarSubLine, arrowType, swatch, hasRange);
        }

        // The AddLine (down/right) button
        if (isMouseOver && scrollBar->subControls & SC_ScrollBarAddLine) {
            QRect scrollBarAddLine =
                    pr->subControlRect(control, scrollBar, SC_ScrollBarAddLine, widget);
            if (!isMouseOver) {
                if (isHorizontal) {
                    scrollBarAddLine.setTop(scrollBarAddLine.bottom() - Ph::SliderDefaultWidth);
                } else {
                    scrollBarAddLine.setLeft(scrollBarAddLine.right() - Ph::SliderDefaultWidth);
                }
            }

            // Arrows
            Qt::ArrowType arrowType;
            if (isHorizontal) {
                arrowType = isLeftToRight ? Qt::RightArrow : Qt::LeftArrow;
            } else {
                arrowType = Qt::DownArrow;
            }
            Ph::drawArrow(painter, scrollBarAddLine, arrowType, swatch, hasRange);
        }
        break;
    }
#endif // QT_CONFIG(slider)
    case CC_ComboBox: {
        auto comboBox = qstyleoption_cast<const QStyleOptionComboBox *>(option);
        if (!comboBox)
            break;
        painter->save();
        bool isSunken = comboBox->state & State_Sunken;
        QRect rect = comboBox->rect;
        QRect downArrowRect =
                proxy()->subControlRect(CC_ComboBox, comboBox, SC_ComboBoxArrow, widget);
        // Draw a line edit
        if (comboBox->editable) {
            Swatchy buttonFill = isSunken ? S_button_pressed : S_button;

            Ph::drawFilledRectNoBorder(painter, rect, Ph::Default_Rounding, swatch, buttonFill);

            if (comboBox->frame) {
                QStyleOptionFrame buttonOption;
                buttonOption.QStyleOption::operator=(*comboBox);
                buttonOption.rect = rect;
                buttonOption.state =
                        (comboBox->state & (State_Enabled | State_MouseOver | State_HasFocus))
                        | State_KeyboardFocusChange;
                if (isSunken) {
                    buttonOption.state |= State_Sunken;
                    buttonOption.state &= ~State_MouseOver;
                }
                proxy()->drawPrimitive(PE_FrameLineEdit, &buttonOption, painter, widget);
            }
        } else {
            QStyleOptionButton buttonOption;
            buttonOption.QStyleOption::operator=(*comboBox);
            buttonOption.rect = rect;
            buttonOption.state = comboBox->state
                    & (State_Enabled | State_MouseOver | State_HasFocus | State_Active
                       | State_KeyboardFocusChange);
            // Combo boxes should be shown to be keyboard interactive if they're
            // focused at all, not just if the user has pressed tab to enter keyboard
            // focus change mode. This is because the up/down arrows can, regardless
            // of having pressed tab, control the combo box selection.
            if (comboBox->state & State_HasFocus)
                buttonOption.state |= State_KeyboardFocusChange;
            if (isSunken) {
                buttonOption.state |= State_Sunken;
                buttonOption.state &= ~State_MouseOver;
            }
            proxy()->drawPrimitive(PE_PanelButtonCommand, &buttonOption, painter, widget);
        }

        if (comboBox->subControls & SC_ComboBoxArrow) {
            int margin = (int)((qreal)qMin(downArrowRect.width(), downArrowRect.height())
                               * Ph::ComboBox_ArrowMarginRatio);
            QRect r = downArrowRect;
            r.adjust(margin, margin, -margin, -margin);
            // Draw the up/down arrow
            Ph::drawArrow(painter, r, Qt::DownArrow, swatch);
        }
        painter->restore();
        break;
    }
#if QT_CONFIG(slider)
    case CC_Slider: {
        auto slider = qstyleoption_cast<const QStyleOptionSlider *>(option);
        if (!slider)
            break;
        const QRect groove = proxy()->subControlRect(CC_Slider, option, SC_SliderGroove, widget);
        const QRect handle = proxy()->subControlRect(CC_Slider, option, SC_SliderHandle, widget);
        bool horizontal = slider->orientation == Qt::Horizontal;
        bool ticksAbove = slider->tickPosition & QSlider::TicksAbove;
        bool ticksBelow = slider->tickPosition & QSlider::TicksBelow;
        Swatchy outlineColor = S_window_outline;
        if (option->state & State_HasFocus && option->state & State_KeyboardFocusChange)
            outlineColor = S_highlight_outline;
        if ((option->subControls & SC_SliderGroove) && groove.isValid()) {
            QRect g0 = groove;
            if (g0.height() > 5)
                g0.adjust(0, 1, 0, -1);
            Ph::PSave saver(painter);
            Swatchy gutterColor = option->state & State_Enabled ? S_button_on : S_window;
            Ph::drawFilledRect(painter, groove, Ph::Default_Rounding, Ph::Default_Thickness, swatch,
                               gutterColor, outlineColor);
        }
        if (option->subControls & SC_SliderTickmarks) {
            Ph::PSave save(painter);
            painter->setPen(swatch.pen(S_window_outline));
            int tickSize = proxy()->pixelMetric(PM_SliderTickmarkOffset, option, widget);
            int available = proxy()->pixelMetric(PM_SliderSpaceAvailable, slider, widget);
            int interval = slider->tickInterval;
            if (interval <= 0) {
                interval = slider->singleStep;
                if (QStyle::sliderPositionFromValue(slider->minimum, slider->maximum, interval,
                                                    available)
                            - QStyle::sliderPositionFromValue(slider->minimum, slider->maximum, 0,
                                                              available)
                    < 3)
                    interval = slider->pageStep;
            }
            if (interval <= 0)
                interval = 1;

            int v = slider->minimum;
            int len = proxy()->pixelMetric(PM_SliderLength, slider, widget);
            while (v <= slider->maximum + 1) {
                if (v == slider->maximum + 1 && interval == 1)
                    break;
                const int v_ = qMin(v, slider->maximum);
                int pos = sliderPositionFromValue(
                                  slider->minimum, slider->maximum, v_,
                                  (horizontal ? slider->rect.width() : slider->rect.height()) - len,
                                  slider->upsideDown)
                        + len / 2;
                int extra = 2 - ((v_ == slider->minimum || v_ == slider->maximum) ? 1 : 0);

                if (horizontal) {
                    if (ticksAbove) {
                        painter->drawLine(pos, slider->rect.top() + extra, pos,
                                          slider->rect.top() + tickSize);
                    }
                    if (ticksBelow) {
                        painter->drawLine(pos, slider->rect.bottom() - extra, pos,
                                          slider->rect.bottom() - tickSize);
                    }
                } else {
                    if (ticksAbove) {
                        painter->drawLine(slider->rect.left() + extra, pos,
                                          slider->rect.left() + tickSize, pos);
                    }
                    if (ticksBelow) {
                        painter->drawLine(slider->rect.right() - extra, pos,
                                          slider->rect.right() - tickSize, pos);
                    }
                }
                // in the case where maximum is max int
                int nextInterval = v + interval;
                if (nextInterval < v)
                    break;
                v = nextInterval;
            }
        }
        // draw handle
        if ((option->subControls & SC_SliderHandle)) {
            bool isPressed = option->state & QStyle::State_Sunken
                    && option->activeSubControls & SC_SliderHandle;
            QRect r = handle;
            Swatchy handleOutline, handleFill, handleSpecular;
            if (option->state & State_HasFocus && option->state & State_KeyboardFocusChange) {
                handleOutline = S_highlight_outline;
            } else {
                handleOutline = S_window_outline;
            }
            if (isPressed) {
                handleFill = S_button_pressed;
                handleSpecular = S_button_pressed_specular;
            } else {
                handleFill = S_button;
                handleSpecular = S_button_specular;
            }
            Ph::PSave save(painter);
            Ph::drawFilledRect(painter, r, Ph::SliderHandle_Rounding, Ph::Default_Thickness, swatch,
                               handleFill, handleOutline);
            r.adjust(Ph::Default_Thickness, Ph::Default_Thickness, -Ph::Default_Thickness,
                     -Ph::Default_Thickness);
            Ph::drawRoundedBorder(painter, r, Ph::SliderHandle_Rounding, Ph::Default_Thickness,
                                  swatch, handleSpecular);
        }
        break;
    }
#endif // QT_CONFIG(slider)
#if QT_CONFIG(toolbutton)
    case CC_ToolButton: {
        auto tbopt = qstyleoption_cast<const QStyleOptionToolButton *>(option);
        QStyleOptionToolButton opt_;
        opt_.QStyleOptionToolButton::operator=(*tbopt);
        opt_.state |= State_AutoRaise;
        QCommonStyle::drawComplexControl(control, &opt_, painter, widget);
        break;
    }
#endif
#if QT_CONFIG(dial)
    case CC_Dial:
        if (auto dial = qstyleoption_cast<const QStyleOptionSlider *>(option))
            Ph::drawDial(dial, painter);
        break;
#endif
    default:
        QCommonStyle::drawComplexControl(control, option, painter, widget);
        break;
    }
}

int QWin11PhantomStyle::pixelMetric(PixelMetric metric, const QStyleOption *option,
                                    const QWidget *widget) const
{
    int val = -1;
    switch (metric) {
    case PM_SliderTickmarkOffset:
        val = 4;
        break;
    case PM_HeaderMargin:
    case PM_ToolTipLabelFrameWidth:
        val = 4;
        break;
    case PM_ButtonMargin:
        val = 6;
        break;
    case PM_ComboBoxFrameWidth:
        val = QWin11Phantom::Frame_Selected_Edge_Width;
        break;
    case PM_ButtonDefaultIndicator:
    case PM_ButtonShiftHorizontal:
        val = 0;
        break;
    case PM_ButtonShiftVertical:
#if QT_CONFIG(toolbutton)
        if (qobject_cast<const QToolButton *>(widget)) {
            return 0;
        }
#endif
        val = 1;
        break;
    case PM_DefaultFrameWidth:
        // Original comment from fusion:
        // Do not dpi-scale because the drawn frame is always exactly 1 pixel thick
        // My note:
        // I seriously doubt, with all of the hacky add-or-remove-1 things
        // everywhere in fusion (and still in phantom), and the fact that fusion is
        // totally broken in high dpi, that this actually holds true.
        return 1;
    case PM_SpinBoxFrameWidth:
        val = QWin11Phantom::Frame_Selected_Edge_Width;
        break;
    case PM_MessageBoxIconSize:
        val = 48;
        break;
    case PM_ScrollBarSliderMin:
        val = 26;
        break;
    case PM_TitleBarHeight:
        val = 24;
        break;
    case PM_ScrollBarExtent:
        val = 10;
        break;
    case PM_SliderThickness:
    case PM_SliderLength:
        val = 20;
        break;
    case PM_DockWidgetTitleMargin:
        val = 1;
        break;
    case PM_MenuVMargin:
    case PM_MenuHMargin:
        val = 4;
        break;
    case PM_MenuPanelWidth:
        val = 1;
        break;
    case PM_MenuBarItemSpacing:
        val = 4;
        break;
    case PM_MenuBarHMargin:
        // option is usually nullptr, use widget instead to get font metrics
        if (!QWin11Phantom::MenuBarLeftMargin || !widget) {
            val = 0;
            break;
        }
        return (int)((qreal)widget->fontMetrics().height()
                     * QWin11Phantom::MenuBar_HorizontalPaddingFontRatio);
    case PM_MenuBarVMargin:
    case PM_MenuBarPanelWidth:
        val = 0;
        break;
    case PM_ToolBarSeparatorExtent:
        val = 9;
        break;
    case PM_ToolBarHandleExtent: {
        int dotLen = (int)QWin11Phantom::dpiScaled(2);
        return dotLen * (3 * 2 - 1);
    }
    case PM_ToolBarItemSpacing:
        val = 1;
        break;
    case PM_ToolBarFrameWidth:
        val = 0;
        break;
    case PM_ToolBarItemMargin:
        val = 1;
        break;
    case PM_ListViewIconSize:
    case PM_SmallIconSize:
#if QT_CONFIG(itemviews)
        if (QWin11Phantom::ItemView_UseFontHeightForDecorationSize && widget
            && qobject_cast<const QAbstractItemView *>(widget)) {
            // QAbstractItemView::viewOptions() always uses nullptr for the
            // styleoption when querying for PM_SmallIconSize. The best we can do is
            // use the font set on the widget itself, which is obviously going to be
            // wrong if the row has a custom font set on it. Hmm.
            return widget->fontMetrics().height();
        }
#endif
        val = 16;
        break;
    case PM_ButtonIconSize: {
        if (option)
            return option->fontMetrics.height();
        if (widget)
            return widget->fontMetrics().height();
        val = 16;
        break;
    }
    case PM_DockWidgetTitleBarButtonMargin:
        val = 2;
        break;
    case PM_TitleBarButtonSize:
        val = 19;
        break;
    case PM_MaximumDragDistance:
        return -1; // Do not dpi-scale because the value is magic
    case PM_TabCloseIndicatorWidth:
    case PM_TabCloseIndicatorHeight:
        val = 16;
        break;
    case PM_TabBarTabHSpace:
        // Contents may clip out horizontally if we don't some extra pixels here or
        // in sizeFromContents for CT_TabBarTab.
        if (!option)
            break;
        return (int)((qreal)option->fontMetrics.height() * QWin11Phantom::TabBar_HPaddingFontRatio)
                + (int)QWin11Phantom::dpiScaled(4);
    case PM_TabBarTabVSpace:
        if (!option)
            break;
        return (int)((qreal)option->fontMetrics.height() * QWin11Phantom::TabBar_VPaddingFontRatio)
                + (int)QWin11Phantom::dpiScaled(2);
    case PM_TabBarTabOverlap:
        val = 1;
        break;
    case PM_TabBarBaseOverlap:
        val = 2;
        break;
    case PM_TabBarIconSize: {
        if (!widget)
            break;
        return widget->fontMetrics().height();
    }
    case PM_SubMenuOverlap:
        val = 0;
        break;
    case PM_DockWidgetHandleExtent:
    case PM_SplitterWidth:
        val = 5;
        break;
    case PM_IndicatorHeight:
    case PM_IndicatorWidth:
    case PM_ExclusiveIndicatorHeight:
    case PM_ExclusiveIndicatorWidth:
        if (option)
            return option->fontMetrics.height();
        if (widget)
            return widget->fontMetrics().height();
        val = 14;
        break;
    case PM_ScrollView_ScrollBarSpacing:
        val = 0;
        break;
    case PM_ScrollView_ScrollBarOverlap:
        val = 0;
        break;
    case PM_TreeViewIndentation: {
        if (widget)
            return widget->fontMetrics().height();
        val = 12;
        break;
    }
    default:
        return QCommonStyle::pixelMetric(metric, option, widget);
    }
    return (int)QWin11Phantom::dpiScaled(val);
}

QSize QWin11PhantomStyle::sizeFromContents(ContentsType type, const QStyleOption *option,
                                           const QSize &size, const QWidget *widget) const
{
    namespace Ph = QWin11Phantom;
    // Cases which do not rely on the parent class to do any work
    switch (type) {
    case CT_RadioButton:
    case CT_CheckBox: {
        auto btn = qstyleoption_cast<const QStyleOptionButton *>(option);
        if (!btn)
            break;
        bool isRadio = type == CT_RadioButton;
        int w = proxy()->pixelMetric(isRadio ? PM_ExclusiveIndicatorWidth : PM_IndicatorWidth, btn,
                                     widget);
        int h = proxy()->pixelMetric(isRadio ? PM_ExclusiveIndicatorHeight : PM_IndicatorHeight,
                                     btn, widget);
        int margins = 0;
        if (!btn->icon.isNull() || !btn->text.isEmpty())
            margins = proxy()->pixelMetric(
                    isRadio ? PM_RadioButtonLabelSpacing : PM_CheckBoxLabelSpacing, option, widget);
        return QSize(size.width() + w + margins, qMax(size.height(), h));
    }
#if QT_CONFIG(menubar)
    case CT_MenuBarItem: {
        int fontHeight = option ? option->fontMetrics.height() : size.height();
        int w = (int)((qreal)fontHeight * Ph::MenuBar_HorizontalPaddingFontRatio);
        int h = (int)((qreal)fontHeight * Ph::MenuBar_VerticalPaddingFontRatio);
        int line = (int)Ph::dpiScaled(1);
        return QSize(size.width() + w * 2, size.height() + h * 2 + line);
    }
#endif
#if QT_CONFIG(menu)
    case CT_MenuItem: {
        auto menuItem = qstyleoption_cast<const QStyleOptionMenuItem *>(option);
        if (!menuItem)
            return size;
        bool hasTabChar = menuItem->text.contains(QLatin1Char('\t'));
        bool hasSubMenu = menuItem->menuItemType == QStyleOptionMenuItem::SubMenu;
        bool isSeparator = menuItem->menuItemType == QStyleOptionMenuItem::Separator;
        int fontMetricsHeight = -1;
        // See notes at CE_MenuItem and SH_ComboBox_Popup for more information
#  if QT_CONFIG(combobox)
        if (Ph::UseQMenuForComboBoxPopup && qobject_cast<const QComboBox *>(widget)) {
            if (!widget->testAttribute(Qt::WA_SetFont))
                fontMetricsHeight = QFontMetrics(qApp->font("QMenu")).height();
        }
#  endif
        if (fontMetricsHeight == -1) {
            fontMetricsHeight = option->fontMetrics.height();
        }
        auto metrics = Ph::MenuItemMetrics::ofFontHeight(fontMetricsHeight);
        // Incoming width is the sum of the visual widths of the main item text and
        // the mnemonic text (if any). To this width we will add the widths of the
        // other features for this menu item -- the icon/checkbox, spacing between
        // icon/text/mnemonic, etc. For cases like separators without any text, we
        // may disregard the width.
        //
        // Height is the text height, probably.
        int w = size.width();
        // Frame
        w += metrics.frameThickness * 2;
        // Left margins don't depend on whether or not we have a submenu arrow.
        // Calculating the right margins requires knowing whether or not the menu
        // item has a submenu arrow.
        w += metrics.leftMargin;
        // Phantom treats every menu item with the same space on the left for a
        // check mark, even if it doesn't have the checkable property.
        w += metrics.checkWidth + metrics.checkRightSpace;

        if (!menuItem->icon.isNull()) {
            // Phantom disregards any user-specified icon sizing at the moment.
            w += metrics.fontHeight;
            w += metrics.iconRightSpace;
        }

        // Tab character is used for separating the shortcut text
        if (hasTabChar)
            w += metrics.mnemonicSpace;
        if (hasSubMenu)
            w += metrics.arrowSpace + metrics.arrowWidth + metrics.rightMarginForArrow;
        else
            w += metrics.rightMarginForText;
        int h;
        if (isSeparator) {
            h = metrics.separatorHeight;
        } else {
            h = metrics.totalHeight;
        }
        if (!menuItem->icon.isNull()) {
#  if QT_CONFIG(combobox)
            if (auto combo = qobject_cast<const QComboBox *>(widget)) {
                h = qMax(combo->iconSize().height() + 2, h);
            }
#  endif // QT_CONFIG(combobox)
        }
        QSize sz;
        sz.setWidth(qMax<int>(w, (int)Ph::dpiScaled(Ph::MenuMinimumWidth)));
        sz.setHeight(h);
        return sz;
    }
    case CT_Menu: {
        if (!Ph::MenuExtraBottomMargin || !option || !widget)
            break;
        // Trick the QMenu into putting a margin only at the bottom by adding extra
        // height to the contents size. We only want to add this tricky space if
        // there is at least more than 1 item in the menu.
        const auto acts = widget->actions();
        if (acts.count() < 2)
            break;
        // We only want to add the tricky space if there's at least 1 separator,
        // otherwise it looks weird.
        bool anySeps = false;
        for (auto act : acts) {
            if (act->isSeparator()) {
                anySeps = true;
                break;
            }
        }
        if (!anySeps)
            break;
        int fheight = option->fontMetrics.height();
        int vmargin = (int)((qreal)fheight * Ph::MenuItem_SeparatorHeightFontRatio) / 2;
        QSize sz = size;
        sz.setHeight(sz.height() + vmargin);
        return sz;
    }
#endif // QT_CONFIG(menu)
#if QT_CONFIG(tabbar)
    case CT_TabBarTab: {
        // Placeholder in case we change this in the future
        return size;
    }
#endif
#if QT_CONFIG(groupbox)
    case CT_GroupBox: {
        // This doesn't seem to get used except once by QGroupBox for
        // minimumSizeHint(). After that, the sizing/layout calculations seem to
        // use the rects given by subControlRect().
        auto opt = qstyleoption_cast<const QStyleOptionGroupBox *>(option);
        if (!opt)
            break;
        // Checkbox and text height already accounted for, but margin between text
        // and frame isn't.
        int xadd = 0;
        int yadd = 0;
        if (opt->subControls & (SC_GroupBoxCheckBox | SC_GroupBoxLabel)) {
            int fontHeight = option->fontMetrics.height();
            yadd += (int)((qreal)fontHeight * QWin11Phantom::GroupBox_LabelBottomMarginFontRatio);
        }
        // We can test for the frame in general, but unfortunately testing to see
        // if it's the 1-line "flat" style or 4-line box/rect "anything else" style
        // doesn't seem to be possible here, only when painting.
        if (opt->subControls & SC_GroupBoxFrame) {
            xadd += 2;
            yadd += 2;
        }
        return QSize(size.width() + xadd, size.height() + yadd);
    }
#endif
#if QT_CONFIG(itemviews)
    case CT_ItemViewItem: {
        auto vopt = qstyleoption_cast<const QStyleOptionViewItem *>(option);
        if (!vopt)
            break;
        QSize sz = QCommonStyle::sizeFromContents(type, option, size, widget);
        // QCommonStyle has a bunch of complicated logic for laying out/calculating
        // rects of view items, which is locked behind a private data guy. In
        // sizeFromContents for CT_ItemViewItem, it unions all of the item row's
        // rects together and then, if the decoration height is exactly the same as
        // the row height, it adds 2 pixels (not dpi scaled) to the height. The
        // comment says it's to prevent "icons from overlapping" but I have no idea
        // how that's supposed to help. And we don't necessarily want those extra 2
        // pixels. Anyway, I don't want to copy and paste all of that code into
        // Phantom and then maintain it. So when Phantom is in the mode where we're
        // basing the item view decoration sizes off of the font size, we'll just
        // take a guess when QCommonStyle has added 2 to the height (because the
        // row height and decoration height are both the font height), and
        // re-remove those two pixels.
#  if 1
        if (QWin11Phantom::ItemView_UseFontHeightForDecorationSize) {
            int fh = vopt->fontMetrics.height();
            if (sz.height() == fh + 2 && vopt->decorationSize.height() == fh) {
                sz.setHeight(fh);
            }
        }
#  endif
        sz += QSize(4, 4); /* margins */
        return sz;
    }
    case CT_HeaderSection: {
        auto hdr = qstyleoption_cast<const QStyleOptionHeader *>(option);
        if (!hdr)
            break;
        // This is pretty crummy. Should also check if we need multi-line support
        // or not.
        bool nullIcon = hdr->icon.isNull();
        int margin = proxy()->pixelMetric(QStyle::PM_HeaderMargin, hdr, widget);
        int iconSize = nullIcon ? 0 : option->fontMetrics.height();
        QSize txt = hdr->fontMetrics.size(Qt::TextSingleLine, hdr->text);
        QSize sz;
        sz.setHeight(margin + qMax(iconSize, txt.height()) + margin);
        sz.setWidth((nullIcon ? 0 : margin) + iconSize + (hdr->text.isNull() ? 0 : margin)
                    + txt.width() + margin);
        if (hdr->sortIndicator != QStyleOptionHeader::None) {
            if (hdr->orientation == Qt::Horizontal)
                sz.rwidth() += sz.height() + margin;
            else
                sz.rheight() += sz.width() + margin;
        }
        return sz;
    }
#endif
    default:
        break;
    }

    // Cases which modify the size given by the parent class
    QSize newSize = QCommonStyle::sizeFromContents(type, option, size, widget);
    switch (type) {
    case CT_PushButton: {
        auto pbopt = qstyleoption_cast<const QStyleOptionButton *>(option);
        if (!pbopt || pbopt->text.isEmpty())
            break;
        int fmheight = pbopt->fontMetrics.height();
        // In high DPI, the measured height of a line of text and the height of the
        // font as reported by QFontMetrics may differ slightly. This is fine,
        // except that we want QPushButtons to have the same height as QComboBoxes,
        // assuming the QPushButton's text isn't multi-line. (QComboBox does not
        // handle multi-line text.) QPushButton will give us a contents height from
        // the actual measured text. QComboBox (and others) will use the font
        // metrics height. Workaround: if there's text and the height is slightly
        // different, adjust by difference in the font metrics height and measured
        // text height. We'll try to avoid clobbering non-standard heights caused
        // by icons or other things.
        if (!pbopt->text.isEmpty()) {
            int vdiff = qAbs(fmheight - size.height());
            // Limit height diff to 2 (scaled) pixels. It's possible that this still
            // clobbers a non-standard height caused by a special-sized icon or
            // something, but there's not much we can do about it.
            if (vdiff != 0 && vdiff <= 2) {
                newSize.rheight() += fmheight - size.height();
            }
        }
        int hpad =
                (int)((qreal)fmheight * QWin11Phantom::PushButton_HorizontalPaddingFontHeightRatio);
        newSize.rwidth() += hpad * 2;
#if QT_CONFIG(dialogbuttonbox)
        if (widget && qobject_cast<const QDialogButtonBox *>(widget->parent())) {
            int dialogButtonMinWidth = (int)QWin11Phantom::dpiScaled(80);
            newSize.rwidth() = qMax(newSize.width(), dialogButtonMinWidth);
        }
#endif
        break;
    }
    case CT_ToolButton:
        newSize += QSize(2, 2);
        break;
    case CT_ComboBox: {
        newSize += QSize(0, 3);
#if QT_CONFIG(combobox)
        auto cb = qstyleoption_cast<const QStyleOptionComboBox *>(option);
        // Non-editable combo boxes have some extra padding on the left side,
        // similar to push buttons. We should account for that here to avoid text
        // being clipped off.
        if (cb) {
            int pad = 0;
            if (cb->editable) {
                pad = (int)Ph::dpiScaled(Ph::LineEdit_ContentsHPad);
            } else {
                pad = (int)Ph::dpiScaled(Ph::ComboBox_NonEditable_ContentsHPad);
            }
            newSize.rwidth() += (pad * 2);
        }
#endif
        break;
    }
    case CT_LineEdit: {
        newSize += QSize(0, 3);
        int pad = (int)Ph::dpiScaled(Ph::LineEdit_ContentsHPad);
        newSize.rwidth() += pad * 2;
        newSize.rheight() += Ph::Frame_Selected_Edge_Width;
        break;
    }
    case CT_SpinBox:
        newSize.rheight() += Ph::Edge_Default_Thickness;
        break;
    case CT_SizeGrip:
        newSize += QSize(4, 4);
        break;
    case CT_MdiControls:
        newSize -= QSize(1, 0);
        break;
    default:
        break;
    }
    return newSize;
}

void QWin11PhantomStyle::polish(QApplication *app)
{
    QCommonStyle::polish(app);
}

void QWin11PhantomStyle::polish(QWidget *widget)
{
    QCommonStyle::polish(widget);
    if (
#if QT_CONFIG(abstractbutton)
            qobject_cast<QAbstractButton *>(widget)
#endif
#if QT_CONFIG(combobox)
            || qobject_cast<QComboBox *>(widget)
#endif
#if QT_CONFIG(progressbar)
            || qobject_cast<QProgressBar *>(widget)
#endif
#if QT_CONFIG(scrollbar)
            || qobject_cast<QScrollBar *>(widget)
#endif
#if QT_CONFIG(splitter)
            || qobject_cast<QSplitterHandle *>(widget)
#endif
#if QT_CONFIG(abstractslider)
            || qobject_cast<QAbstractSlider *>(widget)
#endif
#if QT_CONFIG(spinbox)
            || qobject_cast<QAbstractSpinBox *>(widget)
#endif
            || (widget->inherits("QDockSeparator")) || (widget->inherits("QDockWidgetSeparator"))) {
        widget->setAttribute(Qt::WA_Hover, true);
        widget->setAttribute(Qt::WA_OpaquePaintEvent, false);
    }
}

void QWin11PhantomStyle::polish(QPalette &pal)
{
    QCommonStyle::polish(pal);
}

void QWin11PhantomStyle::unpolish(QWidget *widget)
{
    QCommonStyle::unpolish(widget);
    // Leaving this code here to debug/remove hover stuff if necessary
    if (
#if QT_CONFIG(abstractbutton)
            qobject_cast<QAbstractButton *>(widget)
#endif
#if QT_CONFIG(combobox)
            || qobject_cast<QComboBox *>(widget)
#endif
#if QT_CONFIG(progressbar)
            || qobject_cast<QProgressBar *>(widget)
#endif
#if QT_CONFIG(scrollbar)
            || qobject_cast<QScrollBar *>(widget)
#endif
#if QT_CONFIG(splitter)
            || qobject_cast<QSplitterHandle *>(widget)
#endif
#if QT_CONFIG(abstractslider)
            || qobject_cast<QAbstractSlider *>(widget)
#endif
#if QT_CONFIG(spinbox)
            || qobject_cast<QAbstractSpinBox *>(widget)
#endif
            || (widget->inherits("QDockSeparator")) || (widget->inherits("QDockWidgetSeparator"))) {
        widget->setAttribute(Qt::WA_Hover, false);
    }
}

void QWin11PhantomStyle::unpolish(QApplication *app)
{
    QCommonStyle::unpolish(app);
}

QRect QWin11PhantomStyle::subControlRect(ComplexControl control, const QStyleOptionComplex *option,
                                         SubControl subControl, const QWidget *widget) const
{
    namespace Ph = QWin11Phantom;
    QRect rect = QCommonStyle::subControlRect(control, option, subControl, widget);
    switch (control) {
#if QT_CONFIG(slider)
    case CC_Slider: {
        auto slider = qstyleoption_cast<const QStyleOptionSlider *>(option);
        if (!slider)
            break;
        int tickSize = proxy()->pixelMetric(PM_SliderTickmarkOffset, option, widget);
        switch (subControl) {
        case SC_SliderHandle: {
            if (slider->orientation == Qt::Horizontal) {
                rect.setHeight(proxy()->pixelMetric(PM_SliderThickness));
                rect.setWidth(proxy()->pixelMetric(PM_SliderLength));
                int centerY = slider->rect.center().y() - rect.height() / 2;
                if (slider->tickPosition & QSlider::TicksAbove)
                    centerY += tickSize;
                if (slider->tickPosition & QSlider::TicksBelow)
                    centerY -= tickSize;
                rect.moveTop(centerY);
            } else {
                rect.setWidth(proxy()->pixelMetric(PM_SliderThickness));
                rect.setHeight(proxy()->pixelMetric(PM_SliderLength));
                int centerX = slider->rect.center().x() - rect.width() / 2;
                if (slider->tickPosition & QSlider::TicksAbove)
                    centerX += tickSize;
                if (slider->tickPosition & QSlider::TicksBelow)
                    centerX -= tickSize;
                rect.moveLeft(centerX);
            }
            break;
        }
        case SC_SliderGroove: {
            QPoint grooveCenter = slider->rect.center();
            const int grooveThickness = (int)Ph::dpiScaled(7);
            if (slider->orientation == Qt::Horizontal) {
                rect.setHeight(grooveThickness);
                if (slider->tickPosition & QSlider::TicksAbove)
                    grooveCenter.ry() += tickSize;
                if (slider->tickPosition & QSlider::TicksBelow)
                    grooveCenter.ry() -= tickSize;
            } else {
                rect.setWidth(grooveThickness);
                if (slider->tickPosition & QSlider::TicksAbove)
                    grooveCenter.rx() += tickSize;
                if (slider->tickPosition & QSlider::TicksBelow)
                    grooveCenter.rx() -= tickSize;
            }
            rect.moveCenter(grooveCenter);
            break;
        }
        default:
            break;
        }
        break;
    }
#endif // QT_CONFIG(slider)
#if QT_CONFIG(spinbox)
    case CC_SpinBox: {
        auto cb = qstyleoption_cast<const QStyleOptionSpinBox *>(option);
        if (!cb)
            return QRect();
        int frame = cb->frame ? proxy()->pixelMetric(PM_SpinBoxFrameWidth, cb, widget) : 0;
        QRect rect = option->rect;
        rect.adjust(frame, frame, -frame, -frame);
        int center = rect.height() / 2;

        int dim = qMin(rect.width(), rect.height());
        if (dim < 1)
            return QRect();
        switch (subControl) {
        case SC_SpinBoxFrame:
            rect = cb->rect;
            break;
        case SC_SpinBoxUp:
            if (cb->buttonSymbols == QAbstractSpinBox::NoButtons)
                return QRect();

            rect.setX((rect.x() + rect.width()) - dim + 1);
            rect.setBottom(rect.bottom() - center);
            break;
        case SC_SpinBoxDown:
            if (cb->buttonSymbols == QAbstractSpinBox::NoButtons)
                return QRect();

            rect.setX((rect.x() + rect.width()) - dim + 1);
            rect.setTop(rect.top() + center);
            break;
        case SC_SpinBoxEditField: {
            // Add extra padding if not editable
            if (cb->buttonSymbols != QAbstractSpinBox::NoButtons) {
                rect.setRight(rect.right() - dim);
            }
            rect.setBottom(rect.bottom() - Ph::Frame_Selected_Edge_Width);
            break;
        }
        default:
            break;
        }
        rect = visualRect(cb->direction, cb->rect, rect);
        break;
    }

#endif // QT_CONFIG(spinbox)
#if QT_CONFIG(groupbox)
    case CC_GroupBox: {
        auto groupBox = qstyleoption_cast<const QStyleOptionGroupBox *>(option);
        if (!groupBox)
            break;
        switch (subControl) {
        case SC_GroupBoxFrame:
        case SC_GroupBoxContents: {
            QRect r = option->rect;
            if (groupBox->subControls & (SC_GroupBoxLabel | SC_GroupBoxCheckBox)) {
                int fontHeight = option->fontMetrics.height();
                int topMargin = qMax(pixelMetric(PM_ExclusiveIndicatorHeight), fontHeight);
                topMargin += (int)((qreal)fontHeight * Ph::GroupBox_LabelBottomMarginFontRatio);
                r.setTop(r.top() + topMargin);
            }
            if (subControl == SC_GroupBoxContents && groupBox->subControls & SC_GroupBoxFrame) {
                // Testing against groupBox->features for the frame type doesn't seem
                // to work here.
                r.adjust(1, 1, -1, -1);
            }
            return r;
        }
        case SC_GroupBoxCheckBox:
        case SC_GroupBoxLabel: {
            // Accurate height doesn't matter -- the other group box style
            // implementations also fail with multi-line or too-tall text.
            int textHeight = option->fontMetrics.height();
            // width()/horizontalAdvance() is faster than size() and good enough for
            // us, since we only support a single line of text here anyway.
            int textWidth = Ph::fontMetricsWidth(option->fontMetrics, groupBox->text);
            int indicatorWidth = proxy()->pixelMetric(PM_IndicatorWidth, option, widget);
            int indicatorHeight = proxy()->pixelMetric(PM_IndicatorHeight, option, widget);
            int margin = 0;
            int indicatorRightSpace = textHeight / 3;
            int contentWidth = textWidth;
            if (option->subControls & QStyle::SC_GroupBoxCheckBox) {
                contentWidth += indicatorWidth + indicatorRightSpace;
            }
            int x = margin;
            int y = 0;
            switch (groupBox->textAlignment & Qt::AlignHorizontal_Mask) {
            case Qt::AlignHCenter:
                x += (option->rect.width() - contentWidth) / 2;
                break;
            case Qt::AlignRight:
                x += option->rect.width() - contentWidth;
                break;
            default:
                break;
            }
            int w, h;
            if (subControl == SC_GroupBoxCheckBox) {
                w = indicatorWidth;
                h = indicatorHeight;
                if (textHeight > indicatorHeight) {
                    y = (textHeight - indicatorHeight) / 2;
                }
            } else {
                w = contentWidth;
                h = textHeight;
                if (option->subControls & QStyle::SC_GroupBoxCheckBox) {
                    x += indicatorWidth + indicatorRightSpace;
                    w -= indicatorWidth + indicatorRightSpace;
                }
            }
            return visualRect(option->direction, option->rect, QRect(x, y, w, h));
        }
        default:
            break;
        }
        break;
    }
#endif // QT_CONFIG(groupbox)
#if QT_CONFIG(combobox)
    case CC_ComboBox: {
        auto cb = qstyleoption_cast<const QStyleOptionComboBox *>(option);
        if (!cb)
            return QRect();
        int frame = cb->frame ? proxy()->pixelMetric(PM_ComboBoxFrameWidth, cb, widget) : 0;
        QRect r = option->rect;
        r.adjust(frame, frame, -frame, -frame);
        int dim = qMin(r.width(), r.height());
        if (dim < 1)
            return QRect();
        switch (subControl) {
        case SC_ComboBoxFrame:
            return cb->rect;
        case SC_ComboBoxArrow: {
            r.setX((r.x() + r.width()) - dim + 1);
            return visualRect(option->direction, option->rect, r);
        }
        case SC_ComboBoxEditField: {
            // Add extra padding if not editable
            int pad = 0;
            if (cb->editable) {
                // Line edit padding already added
            } else {
                pad = (int)Ph::dpiScaled(Ph::ComboBox_NonEditable_ContentsHPad);
            }
            r.adjust(pad, 0, -dim, -Ph::Frame_Selected_Edge_Width);
            return visualRect(option->direction, option->rect, r);
        }
        case SC_ComboBoxListBoxPopup: {
            return cb->rect;
        }
        default:
            break;
        }
        break;
    }
#endif
    case CC_TitleBar: {
        auto tb = qstyleoption_cast<const QStyleOptionTitleBar *>(option);
        if (!tb)
            break;
        SubControl sc = subControl;
        QRect &ret = rect;
        const int indent = 3;
        const int controlTopMargin = 3;
        const int controlBottomMargin = 3;
        const int controlWidthMargin = 2;
        const int controlHeight = tb->rect.height() - controlTopMargin - controlBottomMargin;
        const int delta = controlHeight + controlWidthMargin;
        int offset = 0;
        bool isMinimized = tb->titleBarState & Qt::WindowMinimized;
        bool isMaximized = tb->titleBarState & Qt::WindowMaximized;
        switch (sc) {
        case SC_TitleBarLabel:
            if (tb->titleBarFlags & (Qt::WindowTitleHint | Qt::WindowSystemMenuHint)) {
                ret = tb->rect;
                if (tb->titleBarFlags & Qt::WindowSystemMenuHint)
                    ret.adjust(delta, 0, -delta, 0);
                if (tb->titleBarFlags & Qt::WindowMinimizeButtonHint)
                    ret.adjust(0, 0, -delta, 0);
                if (tb->titleBarFlags & Qt::WindowMaximizeButtonHint)
                    ret.adjust(0, 0, -delta, 0);
                if (tb->titleBarFlags & Qt::WindowShadeButtonHint)
                    ret.adjust(0, 0, -delta, 0);
                if (tb->titleBarFlags & Qt::WindowContextHelpButtonHint)
                    ret.adjust(0, 0, -delta, 0);
            }
            break;
        case SC_TitleBarContextHelpButton:
            if (tb->titleBarFlags & Qt::WindowContextHelpButtonHint)
                offset += delta;
            Q_FALLTHROUGH();
        case SC_TitleBarMinButton:
            if (!isMinimized && (tb->titleBarFlags & Qt::WindowMinimizeButtonHint))
                offset += delta;
            else if (sc == SC_TitleBarMinButton)
                break;
            Q_FALLTHROUGH();
        case SC_TitleBarNormalButton:
            if (isMinimized && (tb->titleBarFlags & Qt::WindowMinimizeButtonHint))
                offset += delta;
            else if (isMaximized && (tb->titleBarFlags & Qt::WindowMaximizeButtonHint))
                offset += delta;
            else if (sc == SC_TitleBarNormalButton)
                break;
            Q_FALLTHROUGH();
        case SC_TitleBarMaxButton:
            if (!isMaximized && (tb->titleBarFlags & Qt::WindowMaximizeButtonHint))
                offset += delta;
            else if (sc == SC_TitleBarMaxButton)
                break;
            Q_FALLTHROUGH();
        case SC_TitleBarShadeButton:
            if (!isMinimized && (tb->titleBarFlags & Qt::WindowShadeButtonHint))
                offset += delta;
            else if (sc == SC_TitleBarShadeButton)
                break;
            Q_FALLTHROUGH();
        case SC_TitleBarUnshadeButton:
            if (isMinimized && (tb->titleBarFlags & Qt::WindowShadeButtonHint))
                offset += delta;
            else if (sc == SC_TitleBarUnshadeButton)
                break;
            Q_FALLTHROUGH();
        case SC_TitleBarCloseButton:
            if (tb->titleBarFlags & Qt::WindowSystemMenuHint)
                offset += delta;
            else if (sc == SC_TitleBarCloseButton)
                break;
            ret.setRect(tb->rect.right() - indent - offset, tb->rect.top() + controlTopMargin,
                        controlHeight, controlHeight);
            break;
        case SC_TitleBarSysMenu:
            if (tb->titleBarFlags & Qt::WindowSystemMenuHint) {
                ret.setRect(tb->rect.left() + controlWidthMargin + indent,
                            tb->rect.top() + controlTopMargin, controlHeight, controlHeight);
            }
            break;
        default:
            break;
        }
        ret = visualRect(tb->direction, tb->rect, ret);
        break;
    }
    case CC_ScrollBar: {

        if (const QStyleOptionSlider *scrollbar =
                    qstyleoption_cast<const QStyleOptionSlider *>(option)) {
            switch (subControl) {
            case SC_ScrollBarSlider: {
                if (scrollbar->orientation == Qt::Horizontal)
                    rect.adjust(0, 2, 0, -2);
                else
                    rect.adjust(2, 0, -2, 0);
            }
            default:
                break;
            }
        }
    } break;
    default:
        break;
    }

    return rect;
}

QRect QWin11PhantomStyle::itemPixmapRect(const QRect &r, int flags, const QPixmap &pixmap) const
{
    return QCommonStyle::itemPixmapRect(r, flags, pixmap);
}
void QWin11PhantomStyle::drawItemPixmap(QPainter *painter, const QRect &rect, int alignment,
                                        const QPixmap &pixmap) const
{
    QCommonStyle::drawItemPixmap(painter, rect, alignment, pixmap);
}
QStyle::SubControl QWin11PhantomStyle::hitTestComplexControl(ComplexControl cc,
                                                             const QStyleOptionComplex *opt,
                                                             const QPoint &pt,
                                                             const QWidget *w) const
{
    return QCommonStyle::hitTestComplexControl(cc, opt, pt, w);
}
QPixmap QWin11PhantomStyle::generatedIconPixmap(QIcon::Mode iconMode, const QPixmap &pixmap,
                                                const QStyleOption *opt) const
{
    return QCommonStyle::generatedIconPixmap(iconMode, pixmap, opt);
}

int QWin11PhantomStyle::styleHint(StyleHint hint, const QStyleOption *option, const QWidget *widget,
                                  QStyleHintReturn *returnData) const
{
    switch (hint) {
    case SH_Slider_SnapToValue:
    case SH_PrintDialog_RightAlignButtons:
    case SH_FontDialog_SelectAssociatedText:
    case SH_ComboBox_ListMouseTracking:
    case SH_Slider_StopMouseOverSlider:
    case SH_ScrollBar_MiddleClickAbsolutePosition:
    case SH_TitleBar_AutoRaise:
    case SH_TitleBar_NoBorder:
    case SH_ItemView_ArrowKeysNavigateIntoChildren:
    case SH_ItemView_ChangeHighlightOnFocus:
    case SH_MenuBar_MouseTracking:
    case SH_Menu_MouseTracking:
        return 1;
    case SH_Menu_SupportsSections:
        return 0;
#ifndef Q_OS_MAC
    case SH_MenuBar_AltKeyNavigation:
        return 1;
#endif
#if defined(QT_PLATFORM_UIKIT)
    case SH_ComboBox_UseNativePopup:
        return 1;
#endif
    case SH_ItemView_ShowDecorationSelected:
#if QT_CONFIG(itemviews)
        // QWindowsStyle does this as well -- QCommonStyle seems to have some
        // internal confusion buried within its private implementation of laying
        // out and drawing item views where it can't keep track of what's
        // considered a decoration and what's not. For tree views, if you give 0
        // for ShowDecorationSelected, it applies only to the disclosure indicator
        // and not to the QIcon/pixmap that might be present for the item. So
        // selecting an item in a tree view will have the selection color drawn
        // underneath the icon/pixmap, but not the disclosure indicator. However,
        // in list views, if you give 0 for ShowDecorationSelected, it will *not*
        // draw the selection color underneath the icon/pixmap. There's no way to
        // access this internal logic in QCommonStyle without fully reimplementing
        // the huge mass of stuff for item view layout and drawing. Therefore, the
        // best we can do is at least try to get consistent behavior: if it's a
        // list view, just always return 1 for ShowDecorationSelected.
        if (!QWin11Phantom::ShowItemViewDecorationSelected
            && qobject_cast<const QListView *>(widget))
            return 1;
#endif
        return (int)QWin11Phantom::ShowItemViewDecorationSelected;
    case SH_ItemView_MovementWithoutUpdatingSelection:
        return 1;
#if QT_CONFIG(itemviews)
    case SH_ItemView_ScrollMode:
        return QAbstractItemView::ScrollPerPixel;
#endif
    case SH_ScrollBar_ContextMenu:
#ifdef Q_OS_MAC
        return 0;
#else
        return 1;
#endif
        // Some Linux distros might want to enable this, but it doesn't behave very
        // consistently with varied QPalettes, depending on how the QPA and icons
        // deal with both light and dark themes. It might seem weird to just disable
        // this, but none of (Mac, Windows, BeOS/Haiku) show icons in dialog buttons,
        // and the results on Linux are generally pretty messy -- not sure why it's
        // historically been the default, especially when other button types
        // generally don't have any icons.
    case SH_DialogButtonBox_ButtonsHaveIcons:
        return 0;
    case SH_ScrollBar_Transient:
    case SH_EtchDisabledText:
    case SH_DitherDisabledText:
    case SH_ToolBox_SelectedPageTitleBold:
    case SH_ScrollView_FrameOnlyAroundContents:
    case SH_Menu_AllowActiveAndDisabled:
    case SH_MainWindow_SpaceBelowMenuBar:
    case SH_MessageBox_CenterButtons:
    case SH_RubberBand_Mask:
        return 0;
    case SH_ComboBox_Popup: {
        if (!QWin11Phantom::UseQMenuForComboBoxPopup)
            return 0;
#if QT_CONFIG(combobox)
        // Fusion did this, but we don't because of font bugs (especially in high
        // DPI) with the QMenu that the combo box will create instead of a dropdown
        // view. See notes in CE_MenuItem for more details.
        if (auto cmb = qstyleoption_cast<const QStyleOptionComboBox *>(option))
            return !cmb->editable;
#endif
        return 0;
    }
    case SH_Table_GridLineColor: {
        if (!option)
            return 0;
        auto ph_swatchPtr = QWin11Phantom::getCachedSwatchOfQPalette(
                &d->swatchCache, &d->headSwatchFastKey, option->palette);
        const QWin11Phantom::PhSwatch &swatch = *ph_swatchPtr.data();
        // Qt code in table views for drawing grid lines is broken. See case for
        // CE_ItemViewItem painting for more information.
        return (int)swatch.color(QWin11Phantom::SwatchColors::S_base_divider).rgb();
    }
    case SH_MessageBox_TextInteractionFlags:
        return Qt::TextSelectableByMouse | Qt::LinksAccessibleByMouse;
#if QT_CONFIG(wizard)
    case SH_WizardStyle:
        return QWizard::ClassicStyle;
#endif
    case SH_Menu_SubMenuPopupDelay:
        // Returning 0 will break sloppy submenus even if they're enabled
        return 10;
    case SH_Menu_SloppySubMenus:
        return true;
    case SH_Menu_SubMenuSloppyCloseTimeout:
        return 500;
    case SH_Menu_SubMenuDontStartSloppyOnLeave:
        return 1;
    case SH_Menu_SubMenuSloppySelectOtherActions:
        return 1;
    case SH_Menu_SubMenuUniDirection:
        return 1;
    case SH_Menu_SubMenuUniDirectionFailCount:
        return 1;
    case SH_Menu_SubMenuResetWhenReenteringParent:
        return 0;
#ifdef Q_OS_MAC
    case SH_Menu_FlashTriggeredItem:
        return 1;
    case SH_Menu_FadeOutOnHide:
        return 0;
#endif
    case SH_WindowFrame_Mask:
        return 0;
    case SH_UnderlineShortcut: {
#if defined(Q_OS_MAC)
        return false;
#else
        return true;
#endif
    }
    case SH_Widget_Animate:
        return 0;
    default:
        break;
    }
    return QCommonStyle::styleHint(hint, option, widget, returnData);
}

QRect QWin11PhantomStyle::subElementRect(SubElement sr, const QStyleOption *opt,
                                         const QWidget *w) const
{
    switch (sr) {
    case SE_ProgressBarLabel:
    case SE_ProgressBarContents:
    case SE_ProgressBarGroove:
        return opt->rect;
    case SE_PushButtonFocusRect: {
        QRect r = QCommonStyle::subElementRect(sr, opt, w);
        r.adjust(0, 1, 0, -1);
        return r;
    }
    case SE_DockWidgetTitleBarText: {
        auto titlebar = qstyleoption_cast<const QStyleOptionDockWidget *>(opt);
        if (!titlebar)
            break;
        QRect r = QCommonStyle::subElementRect(sr, opt, w);
        bool verticalTitleBar = titlebar->verticalTitleBar;
        if (verticalTitleBar) {
            r.adjust(0, 0, 0, -4);
        } else {
            if (opt->direction == Qt::LeftToRight)
                r.adjust(4, 0, 0, 0);
            else
                r.adjust(0, 0, -4, 0);
        }
        return r;
    }
#if QT_CONFIG(itemviews)
    case SE_TreeViewDisclosureItem: {
        if (QWin11Phantom::BranchesOnEdge) {
            // Shove it all the way to the left (or right) side, probably outside of
            // the rect it gave us. Old-school.
            QRect rect = opt->rect;
            if (opt->direction != Qt::RightToLeft) {
                rect.moveLeft(0);
                if (rect.width() < rect.height())
                    rect.setWidth(rect.height());
            } else {
                // todo
            }
            return rect;
        }
        break;
    }
#endif
#if QT_CONFIG(lineedit)
    case SE_LineEditContents: {
        QRect r = QCommonStyle::subElementRect(sr, opt, w);
        int pad = (int)QWin11Phantom::dpiScaled(QWin11Phantom::LineEdit_ContentsHPad);
        return r.adjusted(pad, 0, -pad, 0);
    }
#endif
    default:
        break;
    }
    return QCommonStyle::subElementRect(sr, opt, w);
}

// Easiest way is to probably keep your own copy of the header without the
// macro in it. (If there's a smarter way to do this, please let me know.)

// Table header layout reference
// -----------------------------
//
// begin:  QStyleOptionHeader::Beginning;
// mid:    QStyleOptionHeader::Middle;
// end:    QStyleOptionHeader::End;
// one:    QStyleOptionHeader::OnlyOneSection;
// one*:
//   This is specified as QStyleOptionHeader::OnlyOneSection, but the call to
//   drawControl(CE_HeaderSection...) is being performed by an instance of
//   QTableCornerButton, defined in qtableview.cpp as a subclass of
//   QAbstractButton. Only table views can have these corner buttons, and they
//   only appear if there are both at least 1 column and 1 row visible.
//
// Configuration A: A table view with both columns and rows
//
// Configuration B: A list view, or a tree view, or a table view with no rows
// in the data or all rows hidden, such that the corner button is also made
// hidden.
//
// Configuration C: A table view with no columns in the data or all columns
// hidden, such that the corner button is also made hidden.
//
// Configuration A, Left-to-right, 4x4
// [ one*  ][ begin ][ mid ][ mid ][ end ]
// [ begin ]
// [ mid   ]
// [ mid   ]
// [ end   ]
//
// Configuration A, Left-to-right, 2x2
// [ one*  ][ begin ][ end ]
// [ begin ]
// [ end   ]
//
// Configuration A, Left-to-right, 1x1
// [ one* ][ one ]
// [ one  ]
//
// Configuration A, Right-to-left, 4x4
// [ begin ][ mid ][ mid ][ end ][ one*  ]
//                               [ begin ]
//                               [ mid   ]
//                               [ mid   ]
//                               [ end   ]
//
// Configuration A, Right-to-left, 2x2
//               [ begin ][ end ][ one*  ]
//                               [ begin ]
//                               [ end   ]
//
// Configuration A, Right-to-left, 1x1
//                         [ one ][ one* ]
//                                [ one  ]
//
// Configuration B, Left-to-right and right-to-left, 4 columns (table view:
// 4 columns with 0 rows, list/tree view: 4 columns, rows count doesn't matter):
// [ begin ][ mid ][ mid ][ end ]
//
// Configuration B, Left-to-right and right-to-left, 2 columns (table view:
// 2 columns with 0 rows, list/tree view: 2 columns, rows count doesn't matter):
// [ begin ][ end ]
//
// Configuration B, Left-to-right and right-to-left, 1 column (table view:
// 1 column with 0 rows, list view: 1 column, rows count doesn't matter):
// [ one ]
//
// Configuration C, left-to-right and right-to-left, table view with no columns
// and 4 rows:
// [ begin ]
// [ mid   ]
// [ mid   ]
// [ end   ]
//
// Configuration C, left-to-right and right-to-left, table view with no columns
// and 2 rows:
// [ begin ]
// [ end   ]
//
// Configuration C, left-to-right and right-to-left, table view with no columns
// and 1 row:
// [ one ]
