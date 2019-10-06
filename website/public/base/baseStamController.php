<?PHP
/**
* baseStammController
*
* erlaubt das auflisten, anlegen, editieren und löschen von Daten
* einer Datenbank-Tabelle
* 
* URL zum Beispiel: localhost:/config/iotdevice.php?[list|new|edit|save|delete]/key/value
*
* Version 1.1 leichte Veränderung der Methoden-Verarbeitung
*
* @author Andreas Tengicki
* @copyright free, keine urheberrechtliche Schöpfungshöhe
* @version 1.1 (Juni 2018)
*
* @reference baseModel, baseView
* @example iotdevice.php
*
*/

require_once "baseModel.php";
require_once "baseView.php";

class baseStamController
{
  private $model = null;
  public $view  = null;
  
  /**
  * Setzen des Models
  *
  * @param $model zu verwendenes Modell
  */
  public function setModel(baseModel $model)
  {
	  $this->model = $model;
  }
  
  /**
  * Setzen der View
  * wird keine View gesetzt, verwendet er baseView
  *
  * @param $view zu verwendende View
  * @deprecated, bzw. weg damit view ist public
  */
  /*
  public function setView(baseView $view)
  {
	  $this->view = $view;
  }
  */

  /**
  * arbeitet die Anfragen an die Stammdatenbearbeitung ab
  */
  public function handle($request)
  {
    /* Bestimmen der Methode und der Parameter hinter der Methode, vgl Bsp.-URL */	  
    /* Wenn keine Methode definiert, dann "list" */	  
	if (is_null($request->method) || ($request->method == ''))
	  $request->method = 'list';

    //echo '<pre>method: '.$method.'</pre>';
    //echo '<pre>'.print_r($_SERVER,true).'</pre>';

    /* Lade Daten des Models gemäß den Parametern */	
	/* bei Edit wird mit Auswahl auf den Key geladen, damit ein 
	/* update SQL nur für geänderte Felder möglich ist */
    $this->model->load($request->squery);	

    /* Wenn keine View dann baseView */
    if (is_null($this->view))	
      $this->view = new baseView();

    /* Nach save und Delete soll die Liste wieder geladen werden */
    $reload = false;
    if ($request->method == 'save')
      {	  
        $this->model->save($request->post);
	    $reload = true;
	  }
    else if ($request->method == 'del')
      {	  
        $this->model->delete();
	    $reload = true;
	  }

    if ($reload)
      {
		/* Nach save oder delete laden wir wieder die ganze Liste*/
	    $squery = array();
		$this->model->load($squery);	
		$request->method = 'list'; //Damit er wieder die liste ausgibt
      }		
    if ($request->method == 'new')
      $this->view->newRecord($this->model,'');
    else if ($request->method == 'edit')
      $this->view->editRecord($this->model,'');
    else if ($request->method == 'list')
	  $this->view->stammTable($this->model,'');
  }
}  
?> 
 