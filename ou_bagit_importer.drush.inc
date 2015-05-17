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
    'bootstrap' => DRUSH_BOOTSTRAP_DRUPAL_LOGIN,
    'arguments' => array(
      'uri' => 'Location (local or remote) to the bag directory JSON',
    ),
  );

  return $items;
}

function drush_ou_bagit_importer_import_ou_bag($uri) {
  $pathinfo = pathinfo($uri);
  $json_filename = $pathinfo['basename'];
  $directory_basename = preg_replace("/[^a-zA-Z0-9]+/", "", $pathinfo['dirname']);
  $directory = "public://ou_bagit_importer/{$directory_basename}";
  if (file_prepare_directory($directory, FILE_CREATE_DIRECTORY)) {
    @copy($uri, "{$directory}/{$json_filename}");
    ou_bagit_importer_process_json_file($pathinfo['dirname'], $directory, $json_filename);
    // Do stuff
    @unlink("{$directory}/{$json_filename}");
  }
}

function ou_bagit_importer_process_json_file($source_uri, $drupal_directory, $json_filename) {
  $json_data = drupal_json_decode(file_get_contents("{$drupal_directory}/{$json_filename}"));
  if ($json_data) {
    $type = $json_data['recipe']['import'];
    $metadata = array();
    foreach ($json_data['recipe']['metadata'] as $datastream => $path) {
      @copy("{$source_uri}/{$path}", "{$drupal_directory}/{$path}");
      $metadata[$datastream] = "{$drupal_directory}/{$path}";
    }
    if ($json_data['recipe']['import'] == 'book') {
      $object = array(
        'type' => 'islandora:bookCModel',
        'uuid' => $json_data['recipe']['uuid'],
        'metadata' => $metadata,
      );
    }

    ou_bagit_importer_save_book_collection($object, $json_data);
    drupal_set_message(t('Saving book collection @book', array('@book' => $object['uuid'])));
    ou_bagit_importer_process_json_book_pages($source_uri, $drupal_directory, $object, $json_data['recipe']);
    $batch =& batch_get();
    $batch['progressive'] = FALSE;
    drush_backend_batch_process();
  }
  else {
    var_dump("Invalid json data!");
  }
  var_dump($source_uri);
}

function ou_bagit_importer_process_json_book_pages($source_uri, $drupal_directory, $object, $json_data) {
  $batch = array(
    'operations' => array(),
    'finished' => '_ou_bagit_importer_batches_finished',
    'title' => dt('Ingesting BAG...'),
    'init_message' => dt('Preparing to ingest BAG...'),
    'progress_message' => dt('Ingesting BAG'),
    'error_message' => dt('File could not get ingested'),
  );
  foreach ($json_data['pages'] as $key => $page) {
    $batch['operations'][] = array(
      'ou_bagit_importer_batch_import_page',
      array(
        $source_uri,
        $drupal_directory,
        $object,
        $key,
        $page,
        $json_data,
      ),
    );
  }
  batch_set($batch);
}

function ou_bagit_importer_batch_import_page($source_uri, $drupal_directory, $object, $page_number, $page, $json) {
  $file = $page['file'];
  $file_status = @copy("{$source_uri}/{$file}", "{$drupal_directory}/{$file}");
  if ($file_status) {
    $file_hash = md5_file("{$drupal_directory}/{$file}");
    if ($file_hash == $page['sha1']) {
      drupal_set_message(t("Page @page will get ingested here", array('@page' => $file)));
    }
    else {
      drupal_set_message(t("Page @page hash does not match hash in recipe.", array('@page' => $file)), 'warning');
    }
    @unlink("{$drupal_directory}/{$file}");
  }
  else {
    drupal_set_message(t("Could not find @page specified in recipe.", array('@page' => $file)), 'warning');
  }
}

function _ou_bagit_importer_batches_finished() {
  drush_print('Ingest complete!');
}

function ou_bagit_importer_is_path_remote($path) {
    $host = parse_url($path, PHP_URL_HOST);
    if ($host === NULL) {
        return false;
    } else {
        return true;
    }
}
