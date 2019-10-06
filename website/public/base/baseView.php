<?PHP
/**
* baseView
*
* erzeugt Standardausgaben für Listen oder Forms,
* puffert diese in einem Array und nutzt nicht die 
* Output-Bufferung von PHP
* Zum Schluss dann manuelle Ausgabe in ein Layout/Template
* 
* @author Andreas Tengicki
* @copyright free, keine urheberrechtliche Schöpfungshöhe
* @version 1.1 (Juni 2018)
*
* @reference baseStamController, baseModel
* @example iotdevice.php
*
*/
class baseView
{
	
  /**
  * enthällt den gepufferten Content
  */
  private $content = array();
  
  /**
  * Konstruktor, initalisiert das $content-Array
  */
  function __construct()
  {
	$this->content['content'] = '';
  }
  
  /**
  * gibt eine Übersichtsliste, der "druck"baren Felder aus.
  * mit Schaltflächen für neu,edit und delete
  * Achtung, delete ist ohne rückfrage
  *
  * @param model das Model dessen Daten ausgegeben werden
  * @param content, der Bereich in dem der Content gespeichert wird
  */
  public function stammTable($model, $content)
  {
	  if ($content=='') $content = 'content';
	  
	  $this->content[$content] .= '<table border="1">';
	  $fields = $model->listfields;
      $cs = count($fields);
	  $this->content[$content] .=  '<tr><th colspan="'.$cs.'">'.$model->tableName.'</th><th><a href="'.$model->page.'?new/'.$model->primkey.'/0">neu</a></th><th>&nbsp;</th></tr>';
	  $this->content[$content] .=  '<tr>';
	  foreach ($fields as $fn => $label)
	    $this->content[$content] .=  '<th>'.$label.'</th>';
      $this->content[$content] .=  '<th>&nbsp;</th>';
      $this->content[$content] .=  '<th>&nbsp;</th>';
	  foreach ($model->records as $rec)
	    {
	      $this->content[$content] .=  '</tr><tr>';
	      foreach($fields as $fn => $label)
	        $this->content[$content] .=  '<td>'.$rec[$fn].'</td>';
		  $this->content[$content] .= '<td><a href="'.$model->page.'?edit/'.$model->primkey.'/'.$rec[$model->primkey].'">edit</a></td>';	
		  $this->content[$content] .= '<td><a href="'.$model->page.'?del/'.$model->primkey.'/'.$rec[$model->primkey].'">delete</a></td>';	
	    }   
	  $this->content[$content] .= '</tr>';
	  $this->content[$content] .= '</table>';
  }

  /**
  * erzeugt ein Eingabeformular der "druck"baren Felder.
  *
  * @param model das Model dessen Daten ausgegeben werden
  * @param withPrim mit PrimaryKey zum editieren, ohne zur Neuanlage
  * @param content, der Bereich in dem der Content gespeichert wird
  */
  private function printForm($model, $withPrim, $content)
  {
    if ($content=='') $content='content';

    $fields = $model->formfields;
	  
	//echo '<pre>'.print_r($model->metadata,1).'</pre>';
	  
    $cs = count($fields);
	$this->content[$content] .=  '<h2>'.$model->tableName.'</h2>';
	if ($withPrim)
	  {
        $this->content[$content] .=  '<form method="POST" action="'.$model->page.'?save/'.$model->primkey.'/'.$model->records[0][$model->primkey].'">';
        $this->content[$content] .=  '<input type="hidden" name="'.$model->primkey.'" value="'.$model->records[0][$model->primkey].'" />';
	  }
	else
      $this->content[$content] .=  '<form method="POST" action="'.$model->page.'?save">';

    foreach($fields as $fn => $label)
	  {
		//echo '<pre>'.$fn.': '.print_r($model->metadata[$fn],true).'</pre>';
	    $this->content[$content] .= '<label>'.$label.'</label>';
	    $size = $model->metadata[$fn]['Type'];
		$p = strpos($size,'(');
		$size = substr($size,$p+1,strlen($size)-$p-2);
		$val = '';
		if (isset($model->records[0][$fn]))
		  $val = $model->records[0][$fn];
	    if ($model->metadata[$fn]['readonly'])
		  $this->content[$content] .= ' '.$val;
	    else if (array_key_exists($fn,$model->lookups))
		  {
			$this->content[$content] .= '<select name="'.$fn.'" size=1>';  
		    foreach($model->lookups[$fn] as $lookup)
			  {
			    $this->content[$content] .= '<option value="'.$lookup['akey'].'"';
				if ($lookup['abez'] == $val) $this->content[$content] .= ' selected';
				$this->content[$content] .= '>'.$lookup['abez'].'</option>';
			  }
			$this->content[$content] .= '</select>';
		  }
		else
		  $this->content[$content] .=  '<input type="text" name="'.$fn.'" size="'.$size.'" value="'.$val.'"/>';
		$this->content[$content] .=  '<br/>';
	  }  
	if ($withPrim)
	  $this->content[$content] .=  '<button type="submit">speichern</button>';
    else
	  $this->content[$content] .=  '<button type="submit">anlegen</button>';
	$this->content[$content] .=  '</form>';
  }
	  
  /**
  * erzeugt ein Eingabeformular der "druck"baren Felder.
  * zur Neuanlage
  *
  * @param model das Model dessen Daten ausgegeben werden
  * @param content, der Bereich in dem der Content gespeichert wird
  */
  public function newRecord($model,$content)
  {
    $this->printForm($model,false,$content);
  }

  public function addHTML($value,$content)
  {
    if ($content=='') $content='content';
	$this->content[$content] .=  $value;
  }	  
  /**
  * erzeugt ein Eingabeformular der "druck"baren Felder.
  * zum Bearbeiten
  *
  * @param model das Model dessen Daten ausgegeben werden
  * @param content, der Bereich in dem der Content gespeichert wird
  */
  public function editRecord($model,$content)
  {
    $this->printForm($model,true,$content);
  }
  
  /**
  * fügt den Content in das Layout/Template ein
  * Layout wird unter views/<skriptname>.phtml gesucht
  * kann es nicht gefunden werden, nimmt er baseView.phtml
  */
  public function out()
  {
	//Bestimmen des Layout-Verzeichnisses
	$fb = $_SERVER['DOCUMENT_ROOT'].'\\views\\'; 
	//Bestimmen des Skript-Namens
	$fs = explode('/',$_SERVER['SCRIPT_NAME']);
	$fn = $fs[count($fs)-1]	;
	$p = strpos($fn,'.');
	$fn = $fb . substr($fn,0,$p) . '.phtml';
	//Wenn es skript nicht gibt, Default setzten
	if (!file_exists($fn))
	  $fn = $fb . 'baseView.phtml';
	//echo '<pre>'.$fn.'</pre>';
	
	//Layout öffnen und prüfen ob was eingefügt werden kann
	//"content" kommt nach #content
	$h = fopen($fn,'r');
	while (($zeile = fgets($h)) !== false) 
	  {
		foreach (array_keys($this->content) as $key)
		  $zeile = str_replace('#'.$key, $this->content[$key], $zeile);
		echo $zeile;  
      }
	fclose($h);
  }
}
