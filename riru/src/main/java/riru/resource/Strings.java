package riru.resource;

import android.os.SystemProperties;
import android.text.TextUtils;

import java.util.HashMap;
import java.util.Locale;


public class Strings {

    private static final HashMap<String, String[]> STRINGS = new HashMap<>();

    public static final int loaded = 0;
    public static final int files_not_mounted = 1;
    public static final int bad_selinux_rule = 2;
    private static final int COUNT = 4;

    private static Locale locale;

    static {
        try {
            String locale = SystemProperties.get("ro.product.locale");
            if (TextUtils.isEmpty(locale)) {
                locale = SystemProperties.get("persist.sys.locale");
            }
            Strings.locale = Locale.forLanguageTag(locale);
        } catch (Throwable e) {
            Strings.locale = Locale.ENGLISH;
        }

        String[] array;

        array = new String[COUNT];
        array[loaded] = "\uD83D\uDE0B Riru 正常工作中。已载入 %1$d 个模块（%2$s）。";
        array[files_not_mounted] = "\u26A0\uFE0F 文件未被挂载，Magisk 在此设备上损坏。";
        array[bad_selinux_rule] = "\u26A0\uFE0F 此设备上有错误的 SELinux 规则。此问题不应该由 Riru 解决，请参阅模块自述文件以了解更多。";
        Strings.STRINGS.put("zh-CN", array);

        array = new String[COUNT];
        array[loaded] = "\uD83D\uDE0B Riru 正常工作中。已載入 %1$d 個模組（%2$s）。";
        array[files_not_mounted] = "\u26A0\uFE0F 檔案未被掛載，Magisk 在此裝置上損壞。";
        array[bad_selinux_rule] = "\u26A0\uFE0F 此裝置上有錯誤的 SELinux 規則。此問題不應該由 Riru 解決，請參閱模組自述檔案以瞭解更多。";
        Strings.STRINGS.put("zh", array);

        array = new String[COUNT];
        array[loaded] = "\uD83D\uDE0B Riru is working normally. Loaded %1$d modules (%2$s).";
        array[files_not_mounted] = "\u26A0\uFE0F Files are not mounted. Magisk is broken on this device.";
        array[bad_selinux_rule] = "\u26A0\uFE0F Incorrect SELinux found on this device. This issue should not be resolved by Riru, please refer to the module README file for more information.";
        Strings.STRINGS.put("en", array);
    }

    public static String get(int res) {
        return getDefaultString(res, locale);
    }

    private static String[] getStringsMap(Locale locale) {
        String language = locale.getLanguage();
        String region = locale.getCountry();

        // fully match
        locale = new Locale(language, region);
        for (String l : STRINGS.keySet()) {
            if (locale.toString().equals(l.replace('-', '_'))) {
                return STRINGS.get(l);
            }
        }

        // match language only keys
        locale = new Locale(language);
        for (String l : STRINGS.keySet()) {
            if (locale.toString().equals(l)) {
                return STRINGS.get(l);
            }
        }

        // match a language_region with only language
        for (String l : STRINGS.keySet()) {
            if (l.startsWith(locale.toString())) {
                return STRINGS.get(l);
            }
        }

        if (STRINGS.containsKey("en")) {
            return STRINGS.get("en");
        }
        throw new NullPointerException();
    }

    private static String getDefaultString(int res, Locale locale) {
        if (locale == null) {
            locale = Locale.ENGLISH;
        }
        return getStringsMap(locale)[res];
    }

}
