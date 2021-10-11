SELECT  CompanyName, CustomerId, ROUND(exp,2)
FROM
  (SELECT IFNULL(CompanyName, "MISSING_NAME") AS CompanyName, CustomerId, exp, NTILE(4) OVER(ORDER BY CAST(exp AS float)) AS quartile 
    FROM 
      (SELECT "Order".CustomerId, SUM((UnitPrice*Quantity)) AS exp FROM "Order" LEFT OUTER JOIN OrderDetail ON "Order".Id = OrderDetail.OrderId GROUP BY "Order".CustomerId)
    LEFT OUTER JOIN Customer ON CustomerId = Customer.Id)
WHERE quartile=1
ORDER BY CAST(exp AS float);
