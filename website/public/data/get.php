<?php
/**
* get
*
* Entgegennahme Anfragen unserer ioT-Hardware
*
* Im ersten Schritt hatte ich eine Funktion measure/sensordata.php
* Die macht aber so wenig, das wir eine erste Analyse mit späterer
* sinnvoller Authorisierung hier besser zentral machen

* @author Andreas Tengicki
* @copyright free, keine urheberrechtliche Schöpfungshöhe
* @version 1.2 (Januar 2019)
*
*/
$json = file_get_contents('php://input');
$values = json_decode($json,true);

$version = $values['Version']
$dvid = $values['dvid']

if (array_key_exists('values',$values))
  {
    $values = $values['values'];
    $method = $_SERVER['QUERY_STRING']; //?? immer data/get

    $model = new iotSensorModel();
    $model->saveValues($dvid,$method,$values);

    header('Content-Type: application/json');
    echo $model->getJSONDate();
  }
else if (array_key_exists('configs',$values))
  {
    $values = $values['configs'];
    $method = $_SERVER['QUERY_STRING']; //?? immer data/get

    $model = new iotSensorModel();
    $model->saveConfig($dvid,$method,$values);

    header('Content-Type: application/json');
    echo $model->getJSONDate();
  }
  
?>