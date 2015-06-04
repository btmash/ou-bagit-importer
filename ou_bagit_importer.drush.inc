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
  $directory_basename = sha1($pathinfo['dirname']);
  $directory = "public://ou_bagit_importer/{$directory_basename}";
  if (file_prepare_directory($directory, FILE_CREATE_DIRECTORY)) {
    @copy($uri, "{$directory}/{$json_filename}");
    ou_bagit_importer_process_json_file($pathinfo['dirname'], $directory, $json_filename);
    // Do stuff
    @unlink("{$directory}/{$json_filename}");
  }
  else {
    drupal_set_message(t("Could not create directory. You may need to run drush command as sudo"), 'error');
  }
}

function ou_bagit_importer_process_json_file($source_uri, $drupal_directory, $json_filename) {
  $json_data = drupal_json_decode(file_get_contents("{$drupal_directory}/{$json_filename}"));
  if ($json_data) {
    $type = $json_data['recipe']['import'];
    $metadata = array();
    foreach ($json_data['recipe']['metadata'] as $datastream => $path) {
      $metadata[$datastream] = _ou_bagit_importer_copy($source_uri, $drupal_directory, $path);
    }
    if ($json_data['recipe']['import'] == 'book') {
      $object = array(
        'type' => 'islandora:bookCModel',
        'uuid' => $json_data['recipe']['uuid'],
        'metadata' => $metadata,
      );
      ou_bagit_importer_save_book_collection($object, $json_data);
      drupal_set_message(t('Saving book collection @book', array('@book' => $object['uuid'])));
      ou_bagit_importer_process_json_book_pages($source_uri, $drupal_directory, $object, $json_data['recipe']);
    }

    $batch =& batch_get();
    $batch['progressive'] = FALSE;
    drush_backend_batch_process();
  }
  else {
    drupal_set_message(t('Invalid JSON data!'), 'error');
  }
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

function ou_bagit_importer_batch_import_page($source_uri, $drupal_directory, $object, $page_number, $page, $json_data) {
  $file = _ou_bagit_importer_copy($source_uri, $drupal_directory, $page['file']);
  $exif = _ou_bagit_importer_copy($source_uri, $drupal_directory, $page['exif']);
  if ($file) {
    $file_hash = md5_file($file);
    if ($file_hash == $page['sha1']) {
      $object = array(
        'type' => 'islandora:pageCModel',
        'book' => $json_data['uuid'],
        'page_number' => $page_number,
        'file' => $file,
        'exif' => $exif,
        'uuid' => $page['uuid'],
        'metadata' => $metadata,
      );
      drupal_set_message(t("Page @page will get ingested here", array('@page' => $file)));
      ou_bagit_importer_save_book_page($object, $json_data);
    }
    else {
      drupal_set_message(t("Page @page hash does not match hash in recipe.", array('@page' => $file)), 'warning');
    }
    @unlink($file);
    @unlink($exif);
  }
  else {
    drupal_set_message(t("Could not find @page specified in recipe.", array('@page' => $page['file'])), 'warning');
  }
}

function _ou_bagit_importer_batches_finished() {
  drush_print('Ingest complete!');
}

/**
 * Copies file from path to destination directory.
 */
function _ou_bagit_importer_copy($source_uri, $dest_directory, $path) {
  $file = basename($path);
  $source = "{$source_uri}/{$path}";
  if (strpos($path, 'http') === 0) {
    $source = $path;
  }
  $destination = "{$dest_directory}/{$file}";
  $status = @copy($source, $destination);
  if ($status) {
    return $destination;
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
