<?php

/**
 * Implements hook_drush_command().
 */
function ou_bagit_importer_drush_command() {
  $items = array();

  $items['ou_bagit_importer_import_ou_bag'] = array(
    'callback' => 'drush_ou_bagit_importer_import_ou_bag',
    'aliases' => array('oubib'),
    'description' => 'Import an OU bag from a stream',
    'bootstrap' => DRUSH_BOOTSTRAP_DRUPAL_FULL,
    'arguments' => array(
      'uri' => 'Location (local or remote) to the bag directory JSON',
    ),
  );

  return $items;
}

function drush_ou_bagit_importer_import_ou_bag($uri) {
  $pathinfo = pathinfo($uri);
  $filename = $pathinfo['basename'];
  $directory_basename = basename($pathinfo['dirname']);
  $directory = "public://ou_bagit_importer/{$directory_basename}";
  if (file_prepare_directory($directory, FILE_CREATE_DIRECTORY)) {
    // Copy file.
    copy($uri, "{$directory}/$filename");
  }
}

function ou_bagit_importer_is_path_remote($path) {
    $host = parse_url($path, PHP_URL_HOST);
    if ($host === NULL) {
        return false;
    } else {
        return true;
    }
}