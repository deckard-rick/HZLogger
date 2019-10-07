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
  
  public function getConfigJSON($dvkey,&ipadress)
  {
    $erg = array();
    $sql = 'select deviceId, ip from iotdevice where dvkey='.$dvkey;
	$vals = $this->loadSQL($sql);
    $erg['Version'] = 'tgHZ04';
	$erg['DeviceID'] = $vals['devideID'];
	$ipaddress = $vals['ip'];
	$erg['config'] = array();
    $sql = 'select fieldname, wert from iotconfig where dvkey='.$dvkey.' order by fielname';
    $configs = $this->loadSQL($sql);
    foreach($configs as $config)
	  $erg['config'][$config['fieldname']] = $config['wert'];
	  
    return json_encode($erg);
  }	  

  /**
  * Das zu Grunde liegende JSON Model ist, dabei werden die values in values übergeben
  * V3 (07.10.2019)
  * json {version:<version>, DeviceID:<deviceID>, values { 
  *      V1: {id:<id>, anschluss:<anschluss>,sec:<sec>,value:<value>},
  *      V2: ..... }}
  * wobei
  *   <version> : tgHZ04
  *   <deviceID>: HZT001
  *   <id>      : <hexAdressOneWire> || DHT22
  *   anschluss : <konf bei OneWire> || ROOMTEMP || ROOMHUM
  *   <sec>     : Wie alt die Messung ist, da aber bei Bedarf aktuell reported wird, ist das quasi "jetzt"
  *  <value>   : Sensorwert, also Temperatur oder Luftfeuchtigkeit
  *
  */
  public function saveValues($method,$devID,$version,$values)
  {
	$sql = 'select dvkey from iotdevice where dvid=\''.$devID.'\'';
	$dvkey = $this->fetchValue($sql);
	if (isnull($dvkey) || ($dvkey <= 0))
	  return;
	
    $time = date('Y-m-d H:i:s');
	foreach($values as $sensor => $data)
	  {
		if ($data['anschluss'] != '')
	      $sql = 'select sekey from iotsensor where dvkey = '.$dvkey.' and anschluss = \''.$data['anschluss'].'\'';
	    else if ($data['id'] != '')
	      $sql = 'select sekey from iotsensor where dvkey = '.$dvkey.' and id = \''.$data['id'].'\'';
	    else
		  return;
	    $sekey = $this->fetchValue($sql);
	    if (isnull($sekey) || ($sekey <= 0))
	      return;

	    $sql = 'update iotsensor set lastmeasure=\''.$time.'\', lastvalue='.$values['value']
			       ' where sekey = '.$sekey;
		$this->execSQL($sql);

		$sql = 'insert into iotsensorvalue(sekey,zeit,value) '.
			   'values('.$sekey.',\''.$time.'\','.$data['value'].')';
		$this->execSQL($sql);
	  }
  }
  
  public function saveConfig($version,$dvid,$values)
  {
	$sql = 'select dvkey from iotdevice where dvid=\''.$dvid.'\'';
	$dvkey = $this->fetchValue($sql);
	if (isnull($dvkey) || ($dvkey <= 0))
	  return;
	
	$sql = 'update iotconfig set found = \'N\' where dvkey = '.$dvkey;
	$this->execSQL($sql);

	$sql = 'select cfkey, code from iotconfig where dvkey = '.$dvkey.' order by code';
	$configs = $this->loadSQL($sql);
	
	foreach($values as $cfgName => $cfgValue)
	  {
	    $sql = 'select cfkey from iotconfig where fieldname = \''.$cfgName.'\'';
	    $cfkey = $this->fetchValue($sql);

	    if (isnull($dvkey) || ($dvkey <= 0))
	      $sql = 'insert into iotconfig (dvkey,fieldname,wert,found) values ('.$dvkey.',\''.$cfgName.'\',\''.$cfgValue.'\',\'Y\')';
		else
	      $sql = 'update iotconfig set wert=\''.$cfgValue.'\',found=\'Y\' where cfkey = '.$cfkey;
		$this->execSQL($sql);
	  }
	  
	$sql = 'delete from iotconfig where dvkey = '.$dvkey' and  found = \'N\'';
	$this->execSQL($sql);	  
  }
  
}

?>