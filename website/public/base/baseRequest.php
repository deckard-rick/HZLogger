<?PHP
/**
* baseRequest
*
* analysiert zentral die Anfrage
* 
* @author Andreas Tengicki
* @copyright free, keine urheberrechtliche Schöpfungshöhe
* @version 1.2 (Juni 2018)
*
* @reference baseStamController, baseModel
* @example iotdevice.php
*
*/
class baseRequest
{
  public $method;
  public $squery;
  public $post;
  
  function __construct()
  {
    if (isset($_SERVER['QUERY_STRING']))
      $this->squery = explode('/',$_SERVER['QUERY_STRING']);
    else  
	  $this->squery = array();

    $this->method = array_shift($this->squery);
	$this->post = $_POST;
	
	//echo '<pre>REQUEST: '.$this->method.'</pre>';
	//echo '<pre>POST: '.print_r($this->post,true).'</pre>';
  }
}