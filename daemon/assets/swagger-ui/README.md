# Bundled Swagger UI

The distribution assets are vendored from [Swagger UI v5.32.0](https://github.com/swagger-api/swagger-ui/tree/v5.32.0/dist), which supports the daemon's OpenAPI 3.2 contract.

Keep the JavaScript bundles, stylesheet, OAuth redirect files, LICENSE, and NOTICE
from the same upstream release when updating. Preserve the local `index.html`,
`index.css`, `swagger-initializer.js`, and favicons. The initializer loads the
embedded contract from `/openapi.yaml`.
