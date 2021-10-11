select Id, ShipCountry,(case WHEN shipCountry IN ('USA','Mexico','Canada') THEN "NorthAmerica" ELSE "OtherPlace" END)
AS Belong
from 'Order'
where Id >= 15445
order by Id
limit 20;
