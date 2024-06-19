export const Card = ({ children, onClick }) => {
  return (
    <div onClick={onClick} style={{ border: "1px solid black" }}>
      {children}
    </div>
  );
};
