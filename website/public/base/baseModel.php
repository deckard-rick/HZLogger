<?PHP
/**
* baseModel
*
* unterstützt das auflisten, anlegen, editieren und löschen von Daten
* einer Datenbank-Tabelle
*
* @author Andreas Tengicki
* @copyright free, keine urheberrechtliche Schöpfungshöhe
* @version 1.1 (Juni 2018)
*
* @reference baseStamController, baseView
* @example iotdevice.php iotdeviceModel
*
*/
class baseModel
{
  private $db = null;
  public $tableName = '';
  public $page = '';
  public $records = array();
  public $metadata = array();
  public $primKey = '';
  public $lookups = array();
  public $listfields = null;
  public $formfields = null;

  /**
  * baut die Verbindung zur Datenbank auf und bestimmt die Metadaten der verwendeten Tabelle
  * dabei wird automatisch erkannt
  * * primKey der Primary Key
  * * printFields, alle Felder außer der PrimKey, die gelistet werden, bzw in ein Formular kommen
  * * der Kommentar eines mySQL-Feldes ergibt dann die Bezeichnung/Labes des Feldes, wenn vorhanden
  */
  protected function init()
  {
	  date_default_timezone_set('Europe/Berlin');
    $this->db = new mysqli('localhost','root','default','tengicki');

	if ($this->tableName != '')
	  {
	    $sql = 'SHOW FULL COLUMNS FROM '.$this->tableName;
        $res = $this->db->query($sql);
        $metadata = $res->fetch_all(MYSQLI_ASSOC);

		$this->metadata = array();
		$this->primkey = '';
		$fields = array();

		foreach ($metadata as $meta)
		  {
			$fn = $meta['Field'];
			unset($meta['Field']);
		    $this->metadata[$fn] = $meta;
			$this->metadata[$fn]['readonly'] = false;

			if ($meta['Key'] == 'PRI')
			  $this->primkey = $fn;
			if ($meta['Key'] == '')
			  {
			    if ($meta['Comment'] != '')
			      $this->metadata[$fn]['label'] = $meta['Comment'];
			    else
			      $this->metadata[$fn]['label'] = $fn;
			    $fields[$fn] = $this->metadata[$fn]['label'];
			  }
		  }

		$this->formfields = $fields;
		$this->doGetFormFields();
		$this->listfields = $fields;
		$this->doGetListFields();
		//echo '<pre>ListFields: '.print_r($this->listfields,true).'</pre>';
	  }
  }

  /**
  * Destruktor, also Feierabend, DB Verbindung trennen
  */
  function __destruct()
  {
	$this->db->close();
  }

  /**
  * verkürzte Schreibweise, für die Escape Funktion
  *
  * @param $s String der zu escapen ist
  * @return gesicherter String
  */
  private function es($s)
  {
    return $this->db->escape_string($s);
  }

  protected function getLoadSQL()
  {
	  $erg = 'select * from '.$this->tableName;
	  return $erg;
  }

  protected function getLoadOrder()
  {
	  $erg = '';
	  return $erg;
  }

  public function load($squery)
  {
	//echo '<p>load squery: '.print_r($squery,true).'</p>';
	$sql = $this->getLoadSQL();
	$cond = '';
	for($i=0; $i<count($squery); $i=$i+2)
	  $cond .= $this->es($squery[$i]).'='.$this->sqlValue($squery[$i],$squery[$i+1]).' and ';
    if ($cond != '')
	  $sql .= ' where '.substr($cond,0,strlen($cond)-4);

	$order = $this->getLoadOrder();
	if ($order != '')
	  $sql .= ' order by '.$order;

    $this->records = $this->loadSQL($sql);

	for($i=0; $i<count($this->records); $i++)
	  {
		$calcFields = $this->doGetCalcFields($this->records[$i]);
		$this->records[$i] = array_merge($this->records[$i],$calcFields);
		//echo '<pre>'.$i.'</pre>';
		//echo '<pre>'.$this->records[$i].'</pre>';
		//echo '<pre>'.$calcFields.'</pre>';
	  }
  }

  /**
  * Aufbereitung eines Values für SQL
  * String-Values werden in '' eingeschlossen
  * Leere Werte die keine Strings sind, werden auf null übersetzt
  *
  * @param $fn Wenn Wert aus einem Feld, dann welches
  * @param $values, wenn null, dann erster record, wenn Array dann values[$fn] sonst direkt
  * @return SQL-faehiger String
  */
  private function sqlValue($fn,$values)
  {
	/* Wert bestimmen s.o. */
    if (is_null($values))
	  $val = $this->records[0][$fn];
    else if (is_array($values))
	  $val = $values[$fn];
    else
	  $val = $values;
    /* Wert escapen */
    $val = $this->es($val);
	/* In Abhängigkeit vom Typ formatieren */
	$p = strpos($fn,'.');
	if ($p>0) $fn = substr($fn,$p+1);
    $type = $this->metadata[$fn]['Type'];
	if (!(strpos($type,'varchar') === false))
	  $val = '\''.$val.'\'';
    else if ($val == '')
	  $val = 'null';
    return $val;
  }

  /**
  * gibt die "druck"-baren Felder zurück, also die echten zu füllenden
  *
  * @param unused
  * @return Felder die der Benutzer sehen, eingeben kann
  *
  * @todo ggf direkt in Metadaten parken
  */
  protected function doGetListFields()
  {
  }

  protected function doGetFormFields()
  {
  }

  protected function doGetCalcFields($rec)
  {
	  return array();
  }

  /**
  * erzeuge ein INSERT-SQL-Befehl
  * für alle Felder die über Values (meist aus dem $_POST kommen
  * und in den Metadaten existieren (also eine leichte Überprüfung der Zuläsigkeit)
  *
  * @param values, die Werte
  * @return insert-SQL
  */
  private function getInsertSQL($values)
  {
	$ins = 'insert into '.$this->tableName.' (';
	$vals = 'values (';
	foreach ($values as $fn => $val)
	  if (array_key_exists($fn, $this->metadata))
	    {
		  $ins .= $fn.',';  //$fn ist in den Metatdata-Namen und daher sauber, braucht kein escape
		  $vals .= $this->sqlValue($fn,$values).','; //escape ist im sqlValue
	    }
	//Nun fertig zusammenbauen
	return substr($ins,0,strlen($ins)-1).') '.substr($vals,0,strlen($vals)-1).')';
  }

  /**
  * erzeuge ein UPDATE-SQL-Befehl
  * für alle Felder die über Values (meist aus dem $_POST kommen
  * und in den Metadaten existieren (also eine leichte Überprüfung der Zuläsigkeit)
  *
  * Es wird gegen records[0] geprüft, ob sich die Werte veränder haben
  * D.h. der Original-Satz muss zuvor passend geladen werden
  *
  * @param values, die Werte
  * @return update-SQL
  */
  private function getUpdateSQL($values)
  {
	 $changed = false;
     $sql = 'update '.$this->tableName.' set ';
	 foreach ($values as $fn => $val)
	   if (array_key_exists($fn, $this->metadata) && ($fn != $this->primkey))
	     if ((count($this->records)==0) || ($this->records[0][$fn] != $val))
		   {
	         $sql .= $fn.' = '.$this->sqlValue($fn,$values).',';
			 $changed = true;
		   }
	 $erg = '';
	 if ($changed)
	   $erg = substr($sql,0,strlen($sql)-1).' where '.$this->primkey.' = '.$_POST[$this->primkey];
     return $erg;
  }

  /**
  * erzeuge ein DELETE-SQL-Befehl
  *
  * Es wird records[0] verwendet, d.h. der Satz muss existieren und geladen sein
  *
  * @return delete-SQL
  */
  private function getDeleteSQL()
  {
	 $sql = '';
	 if ($this->records[0][$this->primkey] != '')
       $sql = 'delete from '.$this->tableName.' where '.$this->primkey.' = '.$this->records[0][$this->primkey];
     return $sql;
  }

  /**
  * Ausführen eines SQL, mit Test auf leer und Fehlerausgabe
  *
  * @param sql der Befehl
  */
  protected function execSQL($sql)
  {
	if ($sql != '')
      if (!$this->db->query($sql))
        echo '<pre>'.$this->db->error.'</pre>';
  }

  protected function loadSQL($sql)
  {
    //echo '<pre>loadSQL: '.$sql.'</pre>';
    $res = $this->db->query($sql);
    return $res->fetch_all(MYSQLI_ASSOC);
  }

  protected function fetchValue($sql)
  {
    //echo '<pre>fetchSQL: '.$sql.'</pre>';
    $res = $this->db->query($sql);
    $row = $res->fetch_array(MYSQLI_NUM);
    //echo '<pre>ROW:'.print_r($row,1).'</pre>';
	return $row[0];
  }

  /**
  * Speichern von Werte eines Satzes
  * Wenn kein Primary Key Wert dabei ist, dann ist es eine Neuanlage
  *
  * @param values die Werte
  */
  public function save($values)
  {
    //echo '<pre>'.print_r($_POST,true).'</pre>';
	if (!array_key_exists($this->primkey,$values))
	  $sql = $this->getInsertSQL($values);
	else
	  $sql = $this->getUpdateSQL($values);
	echo '<pre>SAVE: '.$sql.'</pre>';
	$this->execSQL($sql);
  }

  /**
  * Löschen des aktuellen Satzes
  */
  public function delete()
  {
    $sql = $this->getDeleteSQL();
    //echo '<pre>DELETE: '.$sql.'</pre>';
	$this->execSQL($sql);

  }

  protected function getTimeArray()
  {
	  $time = array();
	  $time['day'] = date('d');
	  $time['month'] = date('m');
	  $time['year'] = date('Y');
	  $time['hour'] = date('H');
	  $time['minute'] = date('i');
	  $time['second'] = date('s');
	  $erg = array();
	  $erg['time'] = $time;

	  return $erg;
  }

  public function getJSONDate()
  {
	  return json_encode($this->getTimeArray());
  }
}
?>
