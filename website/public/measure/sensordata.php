<?php
/**
* sensorData
*
* Entgegennahme der (Temperatur-)Messwerte unseres Heizungssensors
*
* V1
* Die Daten kommen per JSON { temperatures : { D1 : { value0 : 1.23; value1: 1.34 } } }
* V2
* JSON { dvid : HZT001; sensors : { D1 : { value0 : 1.23; value1: 1.34 } } }
* V3 (07.10.2019)
* json {version:<version>, DeviceID:<deviceID>, values {
*      V1: {id:<id>, anschluss:<anschluss>,sec:<sec>,value:<value>},
*      V2: ..... }}
* V3 (04.12.2019)
* json {version:<version>, DeviceID:<deviceID>, mills:<millis>, values {
*      s0: {id:<id>, sec:<sec>,value:<value>},
*      s1: ..... }}
* wobei
*   <version> : tgHZ04
*   <deviceID>: HZT001
    <millis>  : Lifetime Sonsor in Millisekunden
*   <id>      : <hexAdressOneWire> || DHT22-TEMP || DHT22-HUM
*   <sec>     : Wie alt die Messung ist, da aber bei Bedarf aktuell reported wird, ist das quasi "jetzt"
*   <value>   : Sensorwert, also Temperatur oder Luftfeuchtigkeit
*
* Der Sensor meldet Daten nur, wenn sie sich relevant ändern oder eine reportTime vergangen ist.
* Was relevant ist wird über Schwellwerte je Temperatur-Fühler festgelegt
* Der Sensor filtert, damit wir das Netz und den Server nicht stressem
*
* @author Andreas Tengicki
* @copyright free, keine urheberrechtliche Schöpfungshöhe
* @version 1.2 (Dezember 2019)
*
*/
require_once "../config/iotsensorModel.php";

$json = file_get_contents("php://input");
$values = json_decode($json,true);
$version = $values['Version'];
$dvid = $values['DeviceID'];
$sensors = $values['values'];
$method = $_SERVER['REQUEST_METHOD'];

$model = new iotDeviceModel();
$model->saveValues($method,$version,$dvid,$sensors);

header('Content-Type: application/json');
echo $model->getJSONDate();
?>
