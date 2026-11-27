# Android signing

Google Play builds and the APKs on GitHub releases are signed with the same
app signing key, so either can update an install from the other. Play holds a
copy of that key. Bundles uploaded to Play are signed with a separate upload
key. Signing happens in the signing step; public CI builds the APK and bundle
unsigned and never receives either key.

The public certificates are in `release/keys/`:

- `holder-android-app-signing.pem`: the app signing key
- `holder-android-upload.pem`: the upload key

## Publication gate

An APK is published only if this passes:

```sh
python3 release/scripts/check-android-apk.py Holder-android-<version>.apk \
  --certificate release/keys/holder-android-app-signing.pem
```

It checks the version against root `VERSION` and requires exactly one signer,
the committed app signing certificate.

## Fingerprints

```sh
keytool -printcert -file release/keys/holder-android-app-signing.pem
```

The app signing key's SHA-256 goes in `https://auth.holder.ws/.well-known/assetlinks.json`
for the GitHub sign-in App Link, and its SHA-1 in the release Google Drive
OAuth client for `team.holder.android`. The upload key never reaches a device
and is not registered with either.
