<?PHP
/**
* iotactor
*
* Die verschiedenen ESP8266-Module/Programme brauchen verschiedene Konfigurationswerte
* diese können hier verwaltet werden und per http an das Modul gesandt werden.
*
* @author Andreas Tengicki
* @copyright free, keine urheberrechtliche Schöpfungshöhe
* @version 1.1 (Juni 2018)
*
* @reference iotsensor
*
*/

require_once "iotactorModel.php";
require_once "../base/baseRequest.php";
require_once "../base/baseStamController.php";
  
/* Request analysieren */
$request = new baseRequest();
  
/* Anlegen des Models */
$model = new iotActorModel();

/* Anlage des Controllers */
$controller = new baseStamController();
/* Übergabe des Models an den Controller */
$controller->setModel($model);

/* Übergabe der Verarbeitung an den Controller */
$controller->handle($request);
$controller->view->out();	
?> 
 