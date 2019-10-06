<?PHP
/**
* iotsensorModel
*
* @author Andreas Tengicki
* @copyright free, keine urheberrechtliche Schöpfungshöhe
* @version 1.1 (Juni 2018)
*
* @reference iotdeviceModel
*
*/

require_once "../base/baseModel.php";

class iotSensorModel extends baseModel
{
  function __construct()
  {
	//Setzen des Tabellennames
	$this->tableName = 'iotSensor';
	$this->page = '/config/iotsensor.php';
	//Initialisieren der DB Verbindung mit Metadaten-Bestimmung
	$this->init();
	$this->metadata['lastmeasure']['readonly'] = true;
	$this->metadata['lastvalue0']['readonly'] = true;
	$this->metadata['lastvalue1']['readonly'] = true;
	
	//Hinterlegen der Lookup-Werte für das Abhängige Device
	$sql = 'select dvkey akey, bez abez from iotdevice order by bez';
	$this->lookups['dvkey'] = $this->loadSQL($sql);
  }

  protected function getLoadSQL()
  {
	  //$erg = parent::getLoadSQL();
	  $erg = 'select se.sekey, dv.bez dvkey, se.pin, se.kuerzel, se.bezeichnung, se.measuredelta, se.lastmeasure, se.lastvalue0, se.lastvalue1 '.
	         'from iotsensor se join iotdevice dv on se.dvkey=dv.dvkey ';
	  return $erg;	
  } 
  protected function getLoadOrder()
  {
	  //$erg = parent::getLoadOrder(); 
	  $erg = 'dv.bez, se.pin';
	  return $erg;
  }
  
  public function saveValues($dvid,$method,$values)
  {
	$sql = 'select dvkey from iotdevice where dvid=\''.$dvid.'\'';
	$dvkey = $this->fetchValue($sql);
	
	$sql = 'select sekey, pin, kuerzel from iotsensor where dvkey = '.$dvkey.' order by pin';
	$sensors = $this->loadSQL($sql);
	
    $time = date('Y-m-d H:i:s');
	foreach($values as $sensor => $data)
	  {
		for($i=0; $i<count($sensors); $i++) if ($sensors[$i]['pin'] == $sensor) break;
		if ($i<count($sensors))
		  {
			if (isset($data['v0'])) $value0 = $data['v0']; else $value0 = 'null';
			if (isset($data['v1'])) $value1 = $data['v1']; else $value1 = 'null';

			$sql = 'update iotsensor set lastmeasure=\''.$time.'\', lastvalue0='.$value0.', lastvalue1='.$value1.
			       ' where sekey = '.$sensors[$i]['sekey'];
			$this->execSQL($sql);

			$sql = 'insert into iotsensorvalue(sekey,zeit,dpin,art,value0,value1) '.
			       'values('.$sensors[$i]['sekey'].',\''.$time.'\',\''.$sensor.'\',\''.$sensors[$i]['kuerzel'].'\','.$value0.','.$value1.')';
			$this->execSQL($sql);
		  }
	  }
  }

  public function saveConfig($dvid,$method,$values)
  {
	$sql = 'select dvkey from iotdevice where dvid=\''.$dvid.'\'';
	$dvkey = $this->fetchValue($sql);
	
	$sql = 'select cfkey, code from iotconfig where dvkey = '.$dvkey.' order by code';
	$configs = $this->loadSQL($sql);
	
    $time = date('Y-m-d H:i:s');
	foreach($values as $cfgName => $cfgValue)
	  {
		for($i=0; $i<count($configs); $i++) if ($configs[$i]['code'] == $cfgName) break;
		if ($i<count($sensors))
	      $sql = 'update iotconfig set wert=\''.$cfgValue.'\' where cfkey = '.$configs[$i]['cfkey'];
		else
	      $sql = 'insert into iotconfig (dvkey,code,wert) values ('.$dvkey.',\''.$cfgName.'\',\''.$cfgValue.'\')';
		$this->execSQL($sql);
	  }
  }
}

?>