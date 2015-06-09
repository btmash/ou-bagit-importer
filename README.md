# OU BAGit Importer

A BAG importer following OU specs

## Instructions

    drush -u 1 oubib PATH_TO_RECIPE.json

If there is an error in terms of creating the directory, you may need to fix
the Drupal public files directory permissions or run as sudo

    sudo drush -u 1 oubib PATH_TO_RECIPE.json

You can place drupal user 1 with any other user that has appropriate
credentials to ingest files into Fedora Commons.