<?PHP
/**
* iotsensor
*
* 
*
* @author Andreas Tengicki
* @copyright free, keine urheberrechtliche Schöpfungshöhe
* @version 1.2 (Juni 2018)
*
* @reference iotdevice
*
*/

require_once "iotsensorModel.php";
require_once "../base/baseRequest.php";
require_once "../base/baseStamController.php";

/* Request analysieren */
$request = new baseRequest();

/* Anlegen des Models */
$model = new iotSensorModel();

/* Anlage des Controllers */
$controller = new baseStamController();
/* Übergabe des Models an den Controller */
$controller->setModel($model);
/* Übergabe der Verarbeitung an den Controller */
$controller->handle($request);
/* Ausgabe des Ergebnisses */
$controller->view->out();
?> 
 