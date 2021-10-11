SELECT CompanyName, printf("%.2f", COUNT(CASE WHEN ShippedDate>RequiredDate THEN 1 ELSE null END)/ROUND(COUNT('Order'.Id))*100) 
AS percentage
FROM "Order" LEFT OUTER JOIN Shipper 
WHERE "Order".shipVia = Shipper.Id 
GROUP BY Shipper.Id 
ORDER BY (COUNT(CASE WHEN ShippedDate>RequiredDate THEN 1 ELSE null END)/ROUND(COUNT('Order'.Id))*100) DESC;
