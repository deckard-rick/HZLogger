<?php
/**
* sensorData
*
* Entgegennahme der (Temperatur-)Messwerte unseres Heizungssensors
*
* Die Daten kommen per JSON { temperatures : { D1 : { value0 : 1.23; value1: 1.34 } } }
* NEU
* JSON { dvid : HZT001; sensors : { D1 : { value0 : 1.23; value1: 1.34 } } }
*
* Der Sensor meldet Daten nur, wenn sie sich relevant ändern.
* Was relevant ist wird über Schwellwerte je Temperatur-Fühler festgelegt
* Der Sensor filtert, damit wir das Netz und den Server nicht stressem
*
* @author Andreas Tengicki
* @copyright free, keine urheberrechtliche Schöpfungshöhe
* @version 1.1 (Juni 2018)
*
*/
require_once "../config/iotsensorModel.php";

$json = file_get_contents("php://input");
$values = json_decode($json,true);
$dvid = $values['DeviceID'];
$sensors = $values['values'];
$method = $_SERVER['REQUEST_METHOD'];

$model = new iotSensorModel();
$model->saveValues($dvid,$method,$sensors);

header('Content-Type: application/json');
echo $model->getJSONDate();
?>