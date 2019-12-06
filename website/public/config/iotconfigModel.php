<?PHP
/**
* iotConfigModel
*
* @author Andreas Tengicki
* @copyright free, keine urheberrechtliche Schöpfungshöhe
* @version 1.1 (Juni 2018)
*
* @reference iotSensorModel
*
*/

require_once "../base/baseModel.php";

class iotConfigModel extends baseModel
{
  function __construct()
  {
	//Setzen des Tabellennames
	$this->tableName = 'iotconfig';
	$this->page = '/config/iotconfig.php';
	//Initialisieren der DB Verbindung mit Metadaten-Bestimmung
	$this->init();

	//Hinterlegen der Lookup-Werte für das Abhängige Device
	$sql = 'select dvkey akey, bez abez from iotdevice order by bez';
	$this->lookups['dvkey'] = $this->loadSQL($sql);
  }

  protected function getLoadSQL()
  {
	  //$erg = parent::getLoadSQL();
	  $erg = 'select cf.cfkey, dv.bez dvkey, cf.fieldname, cf.wert from iotconfig cf join iotdevice dv on cf.dvkey=dv.dvkey ';
	  return $erg;
  }
  protected function getLoadOrder()
  {
	  //$erg = parent::getLoadOrder();
	  $erg = 'dv.bez, cf.fieldname';
	  return $erg;
  }

  public function getConfigJSON($dvid)
  {
	$sql = 'select dvkey from iotdevice where dvid=\''.$dvid.'\'';
	$dvkey = $this->fetchValue($sql);

    $erg = $this->getTimeArray();

	$erg['config'] = array();
    $sql = 'select code, wert from iotconfig where dvkey='.$dvkey.' order by code';
    $configs = $this->loadSQL($sql);
    foreach($configs as $config)
	  $erg['config'][$config['code']] = $config['wert'];

	//wir gehen hier erstmal fest von unserem HZT aus, dass muss sich dann später ändern (28.06.2018-tg)
	$erg['hztsensor'] = array();
    $sql = 'select pin, measuredelta from iotsensor where dvkey='.$dvkey.' order by pin';
    $sensors = $this->loadSQL($sql);
    foreach($sensors as $sensor)
	  $erg['hztsensor'][$sensor['pin']] = $sensor['measuredelta'];

	return json_encode($erg);
  }
}

?>
