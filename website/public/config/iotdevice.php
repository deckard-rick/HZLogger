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

	$controller->view->addHTML('<form action="/config/iotdevice.php?getData" method="POST"><input type="hidden" name="dvkey" value="'.$request->squery[1].'"/><button type="submit">von Gerät holen</button></form>','');
 	$controller->view->addHTML('<form action="/config/iotdevice.php?putData" method="POST"><input type="hidden" name="dvkey" value="'.$request->squery[1].'"/><button type="submit">an Gerät senden</button></form>','');
  }

if ($request->method == 'getData')
  {
	echo '<pre>GET DATA</pre>';

	$sql = 'select ip from iotdevice where dvkey ='.$dvkey;
	$url = $this->fetchValue($sql).'://getconfig';

	//Und jetzt holen vom Gerät, das muss ich mit Doku klären. (13.07.2018-tg)
	$values = json_decode($json);
	$model->saveConfig($values['Version'],values['DecideID'],values['configs']);
  }

if ($request->method == 'putData')
  {
	echo '<pre>PUT DATA</pre>';

  //$ipaddress = "";
	//$json = $model->getConfigJSON($request->post['dvkey'],&ipaddress);

	$url = $ipadddress.'://putconfig';

	//Und jetzt senden an das Gerät, das muss ich mit Doku klären. (13.07.2018-tg)
	//header('Content-Type: application/json');
	//echo $model->getConfigJSON('HZT001');
  }

/* Ausgabe des Ergebnisses */
$controller->view->out();

?>
