use crate::deps::*;
use oo7::{Keyring, Secret};
use std::collections::HashMap;

const DOC_LABEL: &str = "org.gnome.Papers.Document";

fn attributes_for_uri(uri: &str) -> HashMap<&str, &str> {
    HashMap::from([("type", "document_password"), ("uri", uri)])
}

pub async fn lookup_password(uri: &str) -> Result<Option<String>, Box<oo7::Error>> {
    let items = Keyring::new()
        .await?
        .search_items(&attributes_for_uri(uri))
        .await?;

    for item in items {
        if item.label().await? == DOC_LABEL {
            return item
                .secret()
                .await
                .map(|p| Some(String::from_utf8_lossy(p.as_ref()).to_string()))
                .map_err(Box::new);
        }
    }

    Ok(None)
}

pub async fn save_password(
    uri: &str,
    password: &str,
    flags: gio::PasswordSave,
) -> Result<(), Box<oo7::Error>> {
    if matches!(flags, gio::PasswordSave::Never) {
        return Ok(());
    }

    Keyring::new()
        .await?
        .create_item(
            DOC_LABEL,
            &attributes_for_uri(uri),
            Secret::text(password),
            true,
        )
        .await
        .map_err(Box::new)
}
