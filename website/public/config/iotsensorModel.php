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
	  $erg = 'select se.sekey, dv.bez dvkey, se.id, se.anschluss, se.bezeichnung, se.lastmeasure, se.lastvalue '.
	         'from iotsensor se join iotdevice dv on se.dvkey=dv.dvkey ';
	  return $erg;
  }

  protected function getLoadOrder()
  {
	  //$erg = parent::getLoadOrder();
	  $erg = 'dv.bez, se.anschluss';
	  return $erg;
  }

  public function saveValues($method,$version,$dvid,$sensors)
  {
    $dvkey = $this->fetchValue('select dvkey from iotdevice where dvid=\''.$dvid.'\'');
    foreach ($sensors as $nr => $sensor)
      {//sensor = array(id=> sec=> value=>)
        $sekey = $this->fetchValue('select sekey from iotsensor where dykey= '.$dvkey.' and id=\''.$sensor['id'].'\'');
        $this->execSQL('update iotsensor set lastmeasure=NOW(), lastvalue = '.$sensor['value'].' where sekey = '.$sekey);
        $this->execSQL('insert into iotsensorvalue(sekey,zeit,value) values ('.$sekey.',NOW(),'.$sensor['value'].')');
      }
  }
}
?>
