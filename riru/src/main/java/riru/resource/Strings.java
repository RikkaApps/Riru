package riru.resource;

import android.os.SystemProperties;
import android.text.TextUtils;

import java.util.HashMap;
import java.util.Locale;


public class Strings {

    private static final HashMap<String, String[]> STRINGS = new HashMap<>();
    private static final Locale LOCALE_DEFAULT = Locale.ENGLISH;

    public static final int loaded = 0;
    public static final int files_not_mounted = 1;
    public static final int bad_selinux_rule = 2;
    public static final int bad_prop = 3;
    public static final int not_loaded = 4;
    private static final int COUNT = 5;

    private static Locale locale;

    static {
        try {
            String locale = SystemProperties.get("persist.sys.locale");
            if (TextUtils.isEmpty(locale)) {
                locale = SystemProperties.get("ro.product.locale");
            }
            Strings.locale = Locale.forLanguageTag(locale);
        } catch (Throwable e) {
            Strings.locale = LOCALE_DEFAULT;
        }

        String[] array;

        // Simplified Chinese
        array = new String[COUNT];
        array[loaded] = "\uD83D\uDE0B Riru 正常工作中。已载入 %1$d 个模块：%2$s。";
        array[files_not_mounted] = "\u26A0\uFE0F 文件未被挂载，Magisk 在此设备上损坏。";
        array[bad_selinux_rule] = "\u26A0\uFE0F 此设备上有错误的 SELinux 规则。此问题不应该由 Riru 解决，请参阅模块自述文件以了解更多。";
        array[bad_prop] = "\u26A0\uFE0F 系统属性错误。请不要使用“优化”模块，因为通过修改属性来优化非常值得怀疑。";
        array[not_loaded] = "\u26A0\uFE0F Riru 未被加载，原因未知。";
        Strings.STRINGS.put("zh-Hans", array);

        // Traditional Chinese
        array = new String[COUNT];
        array[loaded] = "\uD83D\uDE0B Riru 正常工作中。已載入 %1$d 個模組：%2$s。";
        array[files_not_mounted] = "\u26A0\uFE0F 檔案未被掛載，Magisk 在此裝置上損壞。";
        array[bad_selinux_rule] = "\u26A0\uFE0F 此裝置上有錯誤的 SELinux 規則。此問題不應該由 Riru 解決，請參閱模組自述檔案以瞭解更多。";
        array[bad_prop] = "\u26A0\uFE0F 系統屬性錯誤。請不要使用“最佳化”模組，因為透過修改屬性來最佳化非常值得懷疑。";
        array[not_loaded] = "\u26A0\uFE0F Riru 未被載入，原因未知。";
        Strings.STRINGS.put("zh-Hant", array);

        // English
        array = new String[COUNT];
        array[loaded] = "\uD83D\uDE0B Riru is working normally. Loaded %1$d modules: %2$s.";
        array[files_not_mounted] = "\u26A0\uFE0F Files are not mounted. Magisk is broken on this device.";
        array[bad_selinux_rule] = "\u26A0\uFE0F Incorrect SELinux found on this device. This issue should not be resolved by Riru, please refer to the module README file for more information.";
        array[bad_prop] = "\u26A0\uFE0F system property is wrong. Please don't use \"optimize\" modules since it's very questionable to optimize by changing properties.";
        array[not_loaded] = "\u26A0\uFE0F Riru is not loaded and the reason in unknown.";
        Strings.STRINGS.put("en", array);
    }

    public static String get(int res) {
        return getDefaultString(res, locale);
    }

    private static String[] getStringsMap(Locale locale) {
        String language = locale.getLanguage();
        String script = locale.getScript(); // Use getScript() for script code
        String region = locale.getCountry();

        if (!script.isEmpty()) {
            language = language + "-" + script;
        } else if (language.equals("zh")) {
            // Chinese, but no script code provided
            // https://cs.android.com/android/platform/superproject/+/master:frameworks/base/core/res/res/values/locale_config.xml at Line 526~532
            if (region.equalsIgnoreCase("cn") || region.equalsIgnoreCase("sg")) {
                // schinese: China Mainland, Singapore
                language = language + "-Hans";
            } else {
                // tchinese: Taiwan Province of China, Hong Kong Province of China, Macau Province of China
                language = language + "-Hant";
            }
        }

        // fully match (Region first)
        locale = new Locale(language, region);
        for (String l : STRINGS.keySet()) {
            if (locale.toString().equalsIgnoreCase(l)) {
                return STRINGS.get(l);
            }
        }

        // match language only keys
        locale = new Locale(language);
        for (String l : STRINGS.keySet()) {
            if (locale.toString().equalsIgnoreCase(l)) {
                return STRINGS.get(l);
            }
        }

        // match a language_region with only language
        for (String l : STRINGS.keySet()) {
            if (l.startsWith(locale.toString())) {
                return STRINGS.get(l);
            }
        }

        if (STRINGS.containsKey(LOCALE_DEFAULT.getLanguage())) {
            return STRINGS.get(LOCALE_DEFAULT.getLanguage());
        }
        throw new NullPointerException();
    }

    private static String getDefaultString(int res, Locale locale) {
        if (locale == null) {
            locale = LOCALE_DEFAULT;
        }
        return getStringsMap(locale)[res];
    }

}
