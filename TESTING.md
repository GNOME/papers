## Testing and Reporting


Whatever problems you experience with Papers, please test against a current flatpak version, as installed from [Flathub](https://flathub.org/apps/org.gnome.Papers) or the GNOME [Nightly Repository](https://nightly.gnome.org/).
If you can reproduce with these installs, check out our [testing guidlines](#testing) below and file an [issue](https://gitlab.gnome.org/GNOME/papers/-/issues/).

Otherwise Papers itself is most likely not at fault and you should report to your distribution's

### Bug Trackers


* [Arch Linux](https://gitlab.archlinux.org/archlinux/packaging/packages/papers)
* [Alpine](https://gitlab.alpinelinux.org/alpine/aports/-/issues/?sort=created_date&state=all&search=papers&first_page_size=20)
* [Debian](https://bugs.debian.org/cgi-bin/pkgreport.cgi?repeatmerged=no&src=papers)
* [Deepin](https://bbs.deepin.org/en/module/detail/223)
* [Fedora](https://packages.fedoraproject.org/pkgs/papers/papers/)
* [EndeavourOS](https://forum.endeavouros.com/c/general-system/software/)
* [Linux Mint](https://github.com/linuxmint/Cinnamon/issues?q=is%3Aissue%20%20papers)
*  [OpenSuse](https://bugzilla.opensuse.org/)
* [Pop-Os](https://github.com/pop-os/iso/issues?q=papers)
* [PostmarketOS](https://gitlab.postmarketos.org/groups/postmarketOS/-/issues/?sort=created_date&state=all&search=papers&first_page_size=20)
* [Ubuntu](https://launchpad.net/ubuntu/+source/papers/+bugs)
* [ZorinOS](https://zorin.com/os/feedback/)

[Help us](https://gitlab.gnome.org/GNOME/papers/-/issues/new?description_template=none) to improve and extend this list!


## [Testing](testing)

should be done on a current **Devel**opment version, which we assume here. The install is quite easy through [flatpak](https://docs.flatpak.org/en/latest/introduction.html). To test the current main **Devel**opment version, you will want to use the


### [Nightly Repository](https://wiki.gnome.org/Apps/Nightly)

The advantage is, that you can stay on the Nightly Repository and Papers will be updated therefrom, whenever you run `flatpak update` from a terminal. You can add the Nightly Repository via

```
flatpak remote-add --user --if-not-exists gnome-nightly https://nightly.gnome.org/gnome-nightly.flatpakrepo
```

and install the newest **Devel**opment version of Papers via

```
flatpak install --user gnome-nightly org.gnome.Papers.Devel
```

Here some additional information, if you wish to test a

<details><summary>Specific Pipeline</summary>

To test other _concurrent_  **Devel**opment versions of Papers, head over to [Papers' pipelines](https://gitlab.gnome.org/GNOME/papers/-/pipelines), download from the dropdown on the right the correct flatpak, unzip the downloaded file and install via

```
flatpak install --user org.gnome.Papers.Devel.flatpak
```

and confirm the dialog to authorize. To uninstall, just run

```
flatpak uninstall --user org.gnome.Papers.Devel
```

and don't forget to clean up the files!

</details>


### Reporting

If you wish to report an issue, please do so on a latest **Devel**opment version and mention the _exact_ version. To that purpose run from a terminal

```
flatpak info --user org.gnome.Papers.Devel
```

and include the OUTPUT as Information
* in a pre-existing [relevant issue](https://gitlab.gnome.org/GNOME/papers/-/issues) or
* create a [new one](https://gitlab.gnome.org/GNOME/papers/-/issues/new?issuable_template=Default) following the issue template.

Preferrably insert the OUTPUT into a collapsible section, which you can create in Gitlab's text field from the comment menu atop in *plain text mode* or via

````
<details><summary>Information</summary>

```
OUTPUT
```

</details>
````

Note the spacing and **three ticks** before and after OUTPUT. In this way the format of the OUTPUT is preserved, yet collapsible to not clutter the issue tracker.

### Papers Crashes

To address crashes some more information is useful to **debug** the problem. Here we assume, that you have a current **Devel**opment version installed as `--user`, as explained above.

#### Additional Packages

To add the necessary packages for debugging, from a terminal run
```
flatpak install --user gnome-nightly org.gnome.Sdk
```
and
```
flatpak install --user gnome-nightly org.gnome.Sdk.Debug
```
and
```
flatpak install --user gnome-nightly org.gnome.Papers.Devel.Debug
```
Be advised, that these packages consume more 4GB disk space, so you may want to remove them, after having created a debug log file.

#### Creating a Debug Log File

Now, to actually debug Papers, run
```
flatpak run --command=sh --devel --user org.gnome.Papers.Devel
```
then
```
gdb /app/bin/papers
```
and finally
```
run
```
Now Papers should start. **Try to reproduce the crash** or, if necessary, close Papers. In the terminal then type
```
bt full
```
and copy the OUTPUT in a relevant Gitlab [issue](https://gitlab.gnome.org/GNOME/papers/-/issues), again preferrably into a collapsible section, see above.


### Rendering Issues

If a **pdf** file renders incorrectly, e.g. you observe missing or blurry fonts, the culprit might be [Poppler](https://gitlab.freedesktop.org/poppler/poppler), a library Papers relies on. To test, whether that's the case, copy a problematic `FILENAME.pdf` pdf file into your **Download folder** and from a terminal run
```
cd && cd ~/Downloads
```
and
```
mkdir -p ~/Downloads/pdftocairo_test && flatpak run --filesystem=~/Downloads --command=pdftocairo org.gnome.Papers.Devel FILENAME.pdf -png ~/Downloads/pdftocairo_test/test.png`
```

After completion you will find a folder, named `pdftocairo_test` in your Download folder, containing the single pages of `FILENAME.pdf` as png image files. Using your image viewer of choice check, whether the rendering issue is still observable with these image files.

## Reverting the Changes

If you wish to undo the several debugging installations, from a terminal run
```
flatpak uninstall --user org.gnome.Sdk
```
and
```
flatpak uninstall --user org.gnome.Sdk.Debug
```
and
```
flatpak uninstall --user org.gnome.Papers.Devel.Debug
```
and
```
flatpak uninstall --user org.gnome.Papers.Devel
```
and
```
flatpak remote-delete gnome-nightly
```
