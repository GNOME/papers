pub fn gettext_f(format: &str, args: impl IntoIterator<Item = impl AsRef<str>>) -> String {
    let mut s = gettextrs::gettext(format);

    for arg in args {
        s = s.replacen("{}", arg.as_ref(), 1);
    }

    s
}

pub fn ngettext_f(
    format1: &str,
    formatn: &str,
    n: u32,
    args: impl IntoIterator<Item = impl AsRef<str>>,
) -> String {
    let mut s = gettextrs::ngettext(format1, formatn, n);

    for arg in args {
        s = s.replacen("{}", arg.as_ref(), 1);
    }

    s
}

//pub fn freplace(s: String, args: &[(&str, &str)]) -> String {
pub fn gettext_fd(msgid: &str, args: &[(&str, &str)]) -> String {
    let mut s = gettextrs::gettext(msgid);

    for (k, v) in args {
        s = s.replace(&format!("{{{k}}}"), v);
    }
    s
}
