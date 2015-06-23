# OU BAGit Importer

A BAG importer following OU specs

## Instructions

    drush -u 1 oubib --recipe_uri=PATH_TO_RECIPE.json --parent_collection=islandora:root --tmp_dir=/tmp

If there is an error in terms of creating the directory, you may need to fix
the Drupal public files directory permissions or run as sudo

    drush -u 1 oubib --recipe_uri=PATH_TO_RECIPE.json --parent_collection=islandora:root --tmp_dir=/tmp

You can place drupal user 1 with any other user that has appropriate
credentials to ingest files into Fedora Commons.
