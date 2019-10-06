<?PHP
/**
* iotdevice
*
* einfachsten Beispiel einer Stammdatenverarbeitung mit baseStamController,baseModel und baseView
* URL zum Beispiel: localhost:/config/iotdevice.php?[list|new|edit|save|delete]/key/value
*
* @author Andreas Tengicki
* @copyright free, keine urheberrechtliche Schöpfungshöhe
* @version 1.2 (Juni 2018)
*
* @reference baseStamController
*
*/

require_once "iotDeviceModel.php";
require_once "iotactorModel.php";
require_once "iotsensorModel.php";
require_once "iotconfigModel.php";
require_once "../base/baseRequest.php";
require_once "../base/baseStamController.php";
  
/* Request analysieren */
$request = new baseRequest();

/* Anlegen des Models */
$model = new iotdeviceModel();

/* Anlage des Controllers */
$controller = new baseStamController();
/* Übergabe des Models an den Controller */
$controller->setModel($model);
/* Übergabe der Verarbeitung an den Controller */
$controller->handle($request);

if ($request->method == 'show')
  {
    $controller->view->stammTable($model,'');

	$request->squery[0] = 'dv.dvkey';

    $sensor = new iotsensorModel();
    $sensor->load($request->squery);	
    $controller->view->stammTable($sensor,'');

    $actor = new iotactorModel();
    $actor->load($request->squery);	
    $controller->view->stammTable($actor,'');

    $config = new iotConfigModel();
    $config->load($request->squery);	
    $controller->view->stammTable($config,'');
	
	$controller->view->addHTML('<form action="/config/iotdevice.php?sendData" method="POST"><input type="hidden" name="dvkey" value="'.$request->squery[1].'"/><button type="submit">an Gerät senden</button></form>','');
  }
  
if ($request->method == 'sendData')
  {
	echo '<pre>SEND DATA</pre>';

	$json = $model->getConfigJSON($request->post['dvkey']);
	
	$sql = 'select ip from iotdevice where dvkey ='.$dvkey;
	$url = $this->fetchValue($sql).'://sendconfig';
	
	//Und jetzt senden an das Gerät, das muss ich mit Doku klären. (13.07.2018-tg)
	//header('Content-Type: application/json');
	//echo $model->getConfigJSON('HZT001');
  }

/* Ausgabe des Ergebnisses */
$controller->view->out();

?> 
 