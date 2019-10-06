<?PHP
/**
* iotdeviceModel
*
* einfachsten Beispiel einer Stammdatenverarbeitung mit baseStamController,baseModel und baseView
* URL zum Beispiel: localhost:/config/iotdevice.php?[list|new|edit|save|delete]/key/value
*
* Wenn nur eine Tabelle ohne Referenzen bedient werden soll,
* dann genügt die Angabe des Tabellennames für das baseModel
*
* @author Andreas Tengicki
* @copyright free, keine urheberrechtliche Schöpfungshöhe
* @version 1.0 (Juni 2018)
*
* @reference baseModel
*
*/

require_once "../base/baseModel.php";

class iotDeviceModel extends baseModel
{
  function __construct()
  {
	//Setzen des Tabellennames
	$this->tableName = 'iotDevice';
	//Initialisieren der DB Verbindung mit Metadaten-Bestimmung
	$this->init();
  }

  protected function doGetListFields()
  {
	parent::doGetListFields();
	$this->listfields['show'] = 'Show';
  }
  
  protected function doGetCalcFields($rec)
  {
	$calcFields = parent::doGetCalcFields($rec);
	$calcFields['show'] = '<a href="/config/iotdevice.php?show/dvkey/'.$rec['dvkey'].'">Show</a>';
	return $calcFields;
  }
  
  public function getConfigJSON($dvkey)
  {
    $erg = $this->getTimeArray();
    
	$erg['config'] = array();
    $sql = 'select code, wert from iotconfig where dvkey='.$dvkey.' order by code';
    $configs = $this->loadSQL($sql);
    foreach($configs as $config)
	  $erg['config'][$config['code']] = $config['wert'];
	  
	//wir gehen hier erstmal fest von unserem HZT aus, dass muss sich dann später ändern (28.06.2018-tg)
	$erg['sensor'] = array();
    $sql = 'select pin, measuredelta from iotsensor where dvkey='.$dvkey.' order by pin';
    $sensors = $this->loadSQL($sql);
    foreach($sensors as $sensor)
	  $erg['sensor'][$sensor['pin']] = $sensor['measuredelta'];
	
	$erg['actor'] = array();
    $sql = 'select pin, status from iotactor where dvkey='.$dvkey.' order by pin';
    $actors = $this->loadSQL($sql);
    foreach($actors as $actor)
	  $erg['actor'][$actor['pin']] = $actor['status'];

    return json_encode($erg);
  }	  
  
}

?>