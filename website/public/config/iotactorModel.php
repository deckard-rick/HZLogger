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

class iotActorModel extends baseModel
{
  function __construct()
  {
	//Setzen des Tabellennames
	$this->tableName = 'iotactor';
	$this->page = '/config/iotactor.php';
	//Initialisieren der DB Verbindung mit Metadaten-Bestimmung
	$this->init();
	$this->metadata['lastaktiv']['readonly'] = true;
	$this->metadata['lastinaktiv']['readonly'] = true;
	
	//Hinterlegen der Lookup-Werte für das Abhängige Device
	$sql = 'select dvkey akey, bez abez from iotdevice order by bez';
	$this->lookups['dvkey'] = $this->loadSQL($sql);
  }

  protected function getLoadSQL()
  {
	  //$erg = parent::getLoadSQL();
	  $erg = 'select ac.ackey, dv.bez dvkey, ac.pin, ac.kuerzel, ac.bezeichnung, ac.zustand, ac.lastaktiv, ac.lastinaktiv '.
	         'from iotactor ac join iotdevice dv on ac.dvkey=dv.dvkey ';
	  return $erg;	
  } 
  protected function getLoadOrder()
  {
	  //$erg = parent::getLoadOrder(); 
	  $erg = 'dv.bez, ac.pin';
	  return $erg;
  }
}

?>